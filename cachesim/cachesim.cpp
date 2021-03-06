#include "cachesim.h"
#include "binutils.h"
#include "cache_policy_object.h"

#include "processorhandler.h"

#include <QApplication>
#include <QThread>
#include <random>
#include <utility>
#include <cmath>
#include <vector>
#include <assert.h>
#include <iostream>

// #define __SIMDEBUG__

using namespace std;

// Parameters for cache size calculation
extern const unsigned RRIBITWIDTH;
// Parameters for universal hash function in skew associative cache
const unsigned UPRIME = 4294967291u;
const unsigned addr2set[] = { 126, 913, 632, 97 };

// Universal hash function
unsigned AddrWay2Set(unsigned addr, unsigned way, Ripes::CacheSim& cache) {

    // return (cache.getSetIdx(addr) + 3) % cache.getSets();
    //h_ab = (a * k + b) mod m
    return (static_cast<unsigned long>(cache.getSetIdx(addr)) + addr2set[way]) % cache.getSets();

}

namespace Ripes {

CacheSim::CacheSim(QObject* parent) : QObject(parent) {
    connect(ProcessorHandler::get(), &ProcessorHandler::reqProcessorReset, this, &CacheSim::processorReset);

    connect(ProcessorHandler::get(), &ProcessorHandler::runFinished, this, [=] {
        // Given that we are not updating the graphical state of the cache simulator whilst the processor is running,
        // once running is finished, the entirety of the cache view should be reloaded in the graphical view.
        emit hitrateChanged();
        emit cacheInvalidated();
    });

    updateConfiguration();
}

void CacheSim::updateCacheSetReplFields(CacheSet& cacheSet, unsigned int setIdx, unsigned wayIdx, bool isHit) {
    this->m_replPolicyObject->updateCacheSetReplFields(cacheSet, setIdx, wayIdx, isHit);
}

void CacheSim::revertCacheSetReplFields(CacheSet& cacheSet, const CacheWay& oldWay, unsigned wayIdx) {
    this->m_replPolicyObject->revertCacheSetReplFields(cacheSet, oldWay, wayIdx);
}

void CacheSim::setReplacementPolicy(ReplPolicy policy) {
    if (this->m_replPolicyObject != nullptr) {
        delete this->m_replPolicyObject;
        this->m_replPolicyObject = nullptr;
    }
    m_replPolicy = policy;
    this->setReplacementPolicyObject();
    processorReset();
}

void CacheSim::setReplacementPolicyObject() {
    if (this->m_replPolicyObject != nullptr) {
        delete this->m_replPolicyObject;
        this->m_replPolicyObject = nullptr;
    }
    switch (this->m_replPolicy) {
    case ReplPolicy::Random: this->m_replPolicyObject = new RandomPolicy(getWays(), getSets(), getBlocks()); break;
    case ReplPolicy::LRU: this->m_replPolicyObject = new LruPolicy(getWays(), getSets(), getBlocks()); break;
    case ReplPolicy::LRU_LIP: this->m_replPolicyObject = new LruLipPolicy(getWays(), getSets(), getBlocks()); break;
    case ReplPolicy::RRIP: this-> m_replPolicyObject = new RripPolicy(getWays(), getSets(), getBlocks()); break;
    case ReplPolicy::DIP: this-> m_replPolicyObject = new DipPolicy(getWays(), getSets(), getBlocks()); break;
    case ReplPolicy::NoCache: break;
    // TODO: add codes for the cache policy defined by you.
    default: std::cerr << "unknown policy type" << std::endl; break;
    }
}

std::pair<unsigned, CacheWay*> CacheSim::locateEvictionWay(const CacheTransaction& transaction) {
    auto& cacheSet = m_cacheSets[transaction.index.set];

    std::pair<unsigned, CacheWay*> ew;
    ew.first = s_invalidIndex;
    ew.second = nullptr;

    this->m_replPolicyObject->locateEvictionWay(ew, cacheSet, transaction.index.set);

    Q_ASSERT(ew.first != s_invalidIndex && "Unable to locate way for eviction");
    Q_ASSERT(ew.second != nullptr && "Unable to locate way for eviction");
    return ew;
}

CacheWay CacheSim::evictAndUpdate(CacheTransaction& transaction) {
    const unsigned wayIdx = transaction.index.way;
    auto& cacheSet = m_cacheSets[transaction.index.set];
    CacheWay* wayPtr = &cacheSet[wayIdx];

    CacheWay eviction;

    if (!wayPtr->valid) {
        // Record that this was an invalid->valid transition
        transaction.transToValid = true;
    } else {
        // Store the old way info in our eviction trace, in case of rollbacks
        eviction = *wayPtr;
        if (eviction.dirty) {
            // The eviction will result in a writeback
            transaction.isWriteback = true;
        }
    }
    // Invalidate the target way
    *wayPtr = CacheWay();
    // Set required values in way, reflecting the newly loaded address
    wayPtr->valid = true;
    wayPtr->dirty = false;
    // Modified
    //wayPtr->tag = getTag(transaction.address);
    wayPtr->tag = transaction.address;
    transaction.tagChanged = true;

    return eviction;
}

void CacheSim::analyzeCacheAccess(CacheTransaction& transaction) {
    transaction.index.set = getSetIdx(transaction.address);
    transaction.index.block = getBlockIdx(transaction.address);
    transaction.isHit = false;

    if (m_cacheSets.count(transaction.index.set) != 0) {
        for (const auto& way : m_cacheSets.at(transaction.index.set)) {
            if ((way.second.tag == transaction.address) && way.second.valid) {
                transaction.index.way = way.first;
                transaction.isHit = true;
                break;
            }
        }
    }
    if (!transaction.isHit) {
        const auto [wayIdx, wayPtr] = locateEvictionWay(transaction);
        transaction.index.way = wayIdx;
    }
}

void CacheSim::analyzeCacheAccessSkewedCache(CacheTransaction& transaction) {
    if (this->m_type == CacheType::InstrCache) {
        return analyzeCacheAccess(transaction);
    }
    // ---------------------Part 3. TODO ------------------------------
    // Implement the skewed-associative cache

    // Analyze block id
    transaction.index.block = getBlockIdx(transaction.address);
    
    // Get possible locations for this address
    // locations: set, way
    unsigned way_cnt = this->getWays();
    unsigned set_cnt = this->getSets();
    vector<pair<unsigned, unsigned>> locations;
    for (unsigned way_id = 0; way_id < way_cnt; way_id++) {
        locations.push_back(pair<unsigned, unsigned>
            (AddrWay2Set(transaction.address, way_id, *this), way_id));
    }

#ifdef __SIMDEBUG__

    cout << endl;
    cout << "Address: " << transaction.address << endl;
    for (auto location : locations) {
        cout << "Possible location: set " << location.first << ", way: "
            << location.second << endl;
    }

#endif // __SIMDEBUG__

    // CASE1: cache hit
    transaction.isHit = false;
    for (auto location : locations) {    

        // Lazy initialization of corresponding set
        assert(location.first < set_cnt);
        for (int i = 0; i < way_cnt; i++) {
            // NOTE: DO NOT use any map.at() when fetching set
            // That won't initialize entries.
            m_cacheSets[location.first][i];
        }

        // Extract way from corresponding set
        const auto& way = m_cacheSets.at(location.first).at(location.second);

        // Check if it is a hit
        if ((way.tag == transaction.address) && way.valid) {
            transaction.index.set = location.first;
            transaction.index.way = location.second;
            transaction.isHit = true;

#ifdef __SIMDEBUG__

            cout << "Hit at: set " << location.first << ", way "
                << location.second << endl;

#endif // __SIMDEBUG__

            break;
        }

    }

    // CASE2: cache miss
    if (!transaction.isHit) {

        // CASE2.1: cache not fully utilized
        bool exist_invalid_slot = false;
        for (auto location : locations) {
            if (!m_cacheSets.at(location.first).at(location.second).valid) {
                transaction.index.set = location.first;
                transaction.index.way = location.second;
                exist_invalid_slot = true;

#ifdef __SIMDEBUG__

                cout << "Miss and fill in at: set " << location.first << ", way "
                    << location.second << endl;

#endif // __SIMDEBUG__

                break;
            }
        }

        // CASE2.2: cache full, need eviction
        if (!exist_invalid_slot) {

            // Select the entry (set, way) with largest counter
            // The last one if multiple max elements
            unsigned max_counter = 0;
            for (auto location : locations) {
                const auto& way = m_cacheSets.at(location.first).at(location.second);
                if (way.counter >= max_counter) {
                    max_counter = way.counter;
                    transaction.index.set = location.first;
                    transaction.index.way = location.second;
                }
            }

#ifdef __SIMDEBUG__

            cout << "Miss and evict: set " << transaction.index.set << ", way "
                << transaction.index.way << endl;

#endif // __SIMDEBUG__

        }   

    }

#ifdef __SIMDEBUG__

    cout << "Is hit: " << transaction.isHit << endl;

#endif // __SIMDEBUG__

    return;
}

void CacheSim::access(uint32_t address, AccessType type) {
    address = address & ~0b11;  // Disregard unaligned accesses
    CacheTrace trace;
    CacheWay oldWay;
    CacheTransaction transaction;
    transaction.address = address;
    transaction.type = type;

    if (this->m_replPolicy == ReplPolicy::NoCache) {
        sigCacheIsHit.Emit(false);
        return;
    }

    if (this->m_skewPolicy == SkewedAssocPolicy::Skewed) {
        analyzeCacheAccessSkewedCache(transaction); // this should analyze the set and way
    } else {
        analyzeCacheAccess(transaction);
    }

    if (type == AccessType::Write && this->m_wrPolicy == WritePolicy::WriteThrough) {
        sigCacheIsHit.Emit(false);
    } else {
        sigCacheIsHit.Emit(transaction.isHit);
    }

    if (!transaction.isHit) {
        if (type == AccessType::Read ||
            (type == AccessType::Write && getWriteAllocPolicy() == WriteAllocPolicy::WriteAllocate)) {
            oldWay = evictAndUpdate(transaction);
        }
    } else {
        oldWay = m_cacheSets[transaction.index.set][transaction.index.way];
    }

    // === Update dirty and metadata bits ===
    // Initially, we need a check for the case of "write + miss + noWriteAlloc". In this case, we should not update
    // replacement/dirty fields. In all other cases, this is a valid action.
    const bool writeMissNoAlloc =
        !transaction.isHit && type == AccessType::Write && getWriteAllocPolicy() == WriteAllocPolicy::NoWriteAllocate;

    if (!writeMissNoAlloc) {
        // Lazily ensure that the located way has been initialized
        m_cacheSets[transaction.index.set][transaction.index.way];
        if (type == AccessType::Write && getWritePolicy() == WritePolicy::WriteBack) {
            CacheWay& way = m_cacheSets[transaction.index.set][transaction.index.way];
            way.dirty = true;
            way.dirtyBlocks.insert(transaction.index.block);
        }
        updateCacheSetReplFields(m_cacheSets[transaction.index.set], transaction.index.set, transaction.index.way, transaction.isHit);
    } else {
        // In case of a write miss with no write allocate, the value is always written through to memory (a writeback)
        transaction.isWriteback = true;
    }

    // If our WritePolicy is WriteThrough and this access is a write, the transaction will always result in a WriteBack
    if (type == AccessType::Write && getWritePolicy() == WritePolicy::WriteThrough) {
        transaction.isWriteback = true;
    }

    // ===========================

    // At this point, no further changes shall be made to the transaction.
    // We record the transaction as well as a possible eviction
    trace.oldWay = oldWay;
    trace.transaction = transaction;
    pushTrace(trace);
    pushAccessTrace(transaction);

    // === Some sanity checking ===
    // It should never be possible that a read returns an invalid way index
    if (type == AccessType::Read) {
        transaction.index.assertValid();
    }

    // It should never be possible that a write returns an invalid way index if we write-allocate
    if (type == AccessType::Write && getWriteAllocPolicy() == WriteAllocPolicy::WriteAllocate) {
        transaction.index.assertValid();
    }

    // ===========================
    if (writeMissNoAlloc) {
        // There are no graphical changes to perform since nothing is pulled into the cache upon a missed write without
        // write allocation
        return;
    }

    if (isAsynchronouslyAccessed()) {
        return;
    }
    emit dataChanged(&transaction);
}

unsigned CacheSim::getSetIdx(const uint32_t address) const {
    uint32_t maskedAddress = address & m_setMask;
    maskedAddress >>= 2 + getBlockBits();
    return maskedAddress;
}

// Modified
CacheSim::CacheSize CacheSim::getCacheSize() const {
    
    // --------------------- TODO ------------------------------
    // Implement getCacheSize()

    CacheSize size;

    const int entries = getSets() * getWays();

    // Valid bits
    unsigned componentBits = entries;  // 1 bit per entry
    size.components.push_back("Valid bits: " + QString::number(componentBits));
    size.bits += componentBits;

    if (m_wrPolicy == WritePolicy::WriteBack) {
        // Dirty bits
        unsigned componentBits = entries;  // 1 bit per entry
        size.components.push_back("Dirty bits: " + QString::number(componentBits));
        size.bits += componentBits;
    }

    // Capacity calculation for LRU / LRU_LIP control overhead
    if (m_replPolicy == ReplPolicy::LRU || m_replPolicy == ReplPolicy::LRU_LIP) {
        // counter bits
        componentBits = getWaysBits() * entries;
        size.components.push_back("Counter bits: " + QString::number(componentBits));
        size.bits += componentBits;
    }

    // Capacity calculation for DIP control overhead
    if (m_replPolicy == ReplPolicy::DIP) {
        // cacheway counter bits
        componentBits = getWaysBits() * entries;
        size.components.push_back("Counter bits: " + QString::number(componentBits));
        size.bits += componentBits;
        // control bits
        componentBits = 32 * 5 + 1;
        size.components.push_back("Control bits: " + QString::number(componentBits));
        size.bits += componentBits;
    }

    // Capacity calculation for RRIP control overhead
    if (m_replPolicy == ReplPolicy::RRIP) {
        // counter bits
        componentBits = RRIBITWIDTH * entries;
        size.components.push_back("Counter bits: " + QString::number(componentBits));
        size.bits += componentBits;
    }

    // Tag bits
    componentBits = bitcount(m_tagMask) * entries;
    size.components.push_back("Tag bits: " + QString::number(componentBits));
    size.bits += componentBits;

    // Data bits
    componentBits = 32 * entries * getBlocks();
    size.components.push_back("Data bits: " + QString::number(componentBits));
    size.bits += componentBits;

    return size;
}

void CacheSim::setType(CacheSim::CacheType type) {
    m_type = type;
    reassociateMemory();
}

void CacheSim::reassociateMemory() {
    if (m_type == CacheType::DataCache) {
        m_memory.rw = ProcessorHandler::get()->getDataMemory();
    } else if (m_type == CacheType::InstrCache) {
        m_memory.rom = ProcessorHandler::get()->getInstrMemory();
    } else {
        Q_ASSERT(false);
    }
    Q_ASSERT(m_memory.rw != nullptr);
}

unsigned CacheSim::getHits() const {
    if (m_accessTrace.size() == 0) {
        return 0;
    } else {
        auto& trace = m_accessTrace.rbegin()->second;
        return trace.hits;
    }
}

unsigned CacheSim::getMisses() const {
    if (m_accessTrace.size() == 0) {
        return 0;
    } else {
        auto& trace = m_accessTrace.rbegin()->second;
        return trace.misses;
    }
}

unsigned CacheSim::getWritebacks() const {
    if (m_accessTrace.size() == 0) {
        return 0;
    } else {
        auto& trace = m_accessTrace.rbegin()->second;
        return trace.writebacks;
    }
}

double CacheSim::getHitRate() const {
    if (m_accessTrace.size() == 0) {
        return 0;
    } else {
        auto& trace = m_accessTrace.rbegin()->second;
        return static_cast<double>(trace.hits) / (trace.hits + trace.misses);
    }
}


void CacheSim::pushAccessTrace(const CacheTransaction& transaction) {
    // Access traces are pushed in sorted order into the access trace map; indexed by a key corresponding to the cycle
    // of the acces.
    const unsigned currentCycle = ProcessorHandler::get()->getProcessor()->getCycleCount();

    const CacheAccessTrace& mostRecentTrace =
        m_accessTrace.size() == 0 ? CacheAccessTrace() : m_accessTrace.rbegin()->second;

    m_accessTrace[currentCycle] = CacheAccessTrace(mostRecentTrace, transaction);

    if (!isAsynchronouslyAccessed()) {
        emit hitrateChanged();
    }
}

void CacheSim::popAccessTrace() {
    Q_ASSERT(m_accessTrace.size() > 0);
    // The access trace should have an entry
    m_accessTrace.erase(m_accessTrace.rbegin()->first);
    emit hitrateChanged();
}

bool CacheSim::isAsynchronouslyAccessed() const {
    return QThread::currentThread() != QApplication::instance()->thread();
}

void CacheSim::undo() {
    if (m_traceStack.size() == 0)
        return;

    const auto trace = popTrace();
    popAccessTrace();

    const auto& oldWay = trace.oldWay;
    const auto& transaction = trace.transaction;
    const unsigned& setIdx = trace.transaction.index.set;
    const unsigned& blockIdx = trace.transaction.index.block;
    const unsigned& wayIdx = trace.transaction.index.way;
    auto& set = m_cacheSets.at(setIdx);
    auto& way = set.at(wayIdx);

    // Case 1: A cache way was transitioned to valid. In this case, we simply invalidate the cache way
    if (trace.transaction.transToValid) {
        // Invalidate the way
        Q_ASSERT(m_cacheSets.at(setIdx).count(wayIdx) != 0);
        way = CacheWay();
    }
    // Case 2: A miss occured on a valid entry. In this case, we have to restore the old way, which was evicted
    // - Restore the old entry which was evicted
    else if (!trace.transaction.isHit) {
        way = oldWay;
    }
    // Case 3: Else, it was a cache hit; Revert replacement fields and dirty blocks
    way.dirtyBlocks = oldWay.dirtyBlocks;
    revertCacheSetReplFields(set, oldWay, wayIdx);

    // Notify that changes to the way has been performed
    emit wayInvalidated(setIdx, wayIdx);

    // Finally, re-emit the transaction which occurred in the previous cache access to update the cache
    // highlighting state
    if (m_traceStack.size() > 0) {
        emit dataChanged(&m_traceStack.begin()->transaction);
    } else {
        emit dataChanged(nullptr);
    }
}

CacheSim::CacheTrace CacheSim::popTrace() {
    Q_ASSERT(m_traceStack.size() > 0);
    auto val = m_traceStack.front();
    m_traceStack.pop_front();
    return val;
}

void CacheSim::pushTrace(const CacheTrace& eviction) {
    m_traceStack.push_front(eviction);
    if (m_traceStack.size() > vsrtl::core::ClockedComponent::reverseStackSize()) {
        m_traceStack.pop_back();
    }
}

uint32_t CacheSim::buildAddress(unsigned tag, unsigned setIdx, unsigned blockIdx) const {
    uint32_t address = 0;
    address |= tag << (2 /*byte offset*/ + getBlockBits() + getSetBits());
    address |= setIdx << (2 /*byte offset*/ + getBlockBits());
    address |= blockIdx << (2 /*byte offset*/);
    return address;
}

unsigned CacheSim::getTag(const uint32_t address) const {
    uint32_t maskedAddress = address & m_tagMask;
    maskedAddress >>= 2 + getBlockBits() + getSetBits();
    return maskedAddress;
}

unsigned CacheSim::getBlockIdx(const uint32_t address) const {
    uint32_t maskedAddress = address & m_blockMask;
    maskedAddress >>= 2;
    return maskedAddress;
}

const CacheSet* CacheSim::getSet(unsigned idx) const {
    if (m_cacheSets.count(idx)) {
        return &m_cacheSets.at(idx);
    } else {
        return nullptr;
    }
}

void CacheSim::processorWasClocked() {
    // We do not access cache per clock due to memory stalls
    // The cache access is triggered by the signal sent from memory module.
    return;
}

void CacheSim::processorWasReversed() {
    if (m_accessTrace.size() == 0) {
        // Nothing to reverse
        return;
    }
    const unsigned cycleToUndo = ProcessorHandler::get()->getProcessor()->getCycleCount() + 1;
    if (m_accessTrace.rbegin()->first != cycleToUndo) {
        // No cache access in this cycle
        return;
    }
    // It is now safe to undo the cycle at the top of our access stack(s).
    undo();
}

void CacheSim::updateConfiguration() {
    // Cache configuration changed. Reset all state
    m_cacheSets.clear();
    m_accessTrace.clear();
    m_traceStack.clear();

    // Recalculate masks
    int bitoffset = 2;  // 2^2 = 4-byte offset (32-bit words in cache)

    m_blockMask = generateBitmask(getBlockBits()) << bitoffset;
    bitoffset += getBlockBits();

    m_setMask = generateBitmask(getSetBits()) << bitoffset;
    bitoffset += getSetBits();

    m_tagMask = generateBitmask(32 - bitoffset) << bitoffset;

    // Reset the graphical view & processor
    emit configurationChanged();

    if (m_memory.rw || m_memory.rom) {
        // Reload the initial (cycle 0) state of the processor. This is necessary to reflect ie. the instruction which
        // is loaded from the instruction memory in cycle 0.
        processorWasClocked();
    }
}

void CacheSim::processorReset() {
    /** see comment of m_isResetting */
    if (m_isResetting) {
        return;
    }

    m_isResetting = true;
    // The processor might have changed. Since our signals/slot library cannot check for existing connection, we do the
    // safe, slightly redundant, thing of disconnecting and reconnecting the VSRTL design update signals.
    reassociateMemory();
    auto* proc = ProcessorHandler::get()->getProcessorNonConst();
    proc->designWasClocked.Connect(this, &CacheSim::processorWasClocked);
    proc->designWasReversed.Connect(this, &CacheSim::processorWasReversed);
    proc->designWasReset.Connect(this, &CacheSim::processorReset);

    updateConfiguration();
    m_isResetting = false;
}

void CacheSim::setBlocks(unsigned blocks) {
    m_blocks = blocks;
    setReplacementPolicyObject();
    processorReset();
}
void CacheSim::setSets(unsigned sets) {
    m_sets = sets;
    setReplacementPolicyObject();
    processorReset();
}
void CacheSim::setWays(unsigned ways) {
    m_ways = ways;
    setReplacementPolicyObject();
    processorReset();
}

void CacheSim::setWritePolicy(WritePolicy policy) {
    m_wrPolicy = policy;
    processorReset();
}

void CacheSim::setWriteAllocatePolicy(WriteAllocPolicy policy) {
    m_wrAllocPolicy = policy;
    processorReset();
}

void CacheSim::setPreset(const CachePreset& preset) {
    m_blocks = preset.blocks;
    m_ways = preset.ways;
    m_sets = preset.sets;
    m_wrPolicy = preset.wrPolicy;
    m_wrAllocPolicy = preset.wrAllocPolicy;
    m_replPolicy = preset.replPolicy;

    processorReset();
}

}  // namespace Ripes
