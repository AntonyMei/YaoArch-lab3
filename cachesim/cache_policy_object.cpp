#include "cache_policy_object.h"
#include "cache_organize_component.h"
#include <iostream>


namespace Ripes {

/******************************************************************************/
// Random policy

void RandomPolicy::locateEvictionWay(std::pair<unsigned, CacheWay*>& ew,
                                     CacheSet& cacheSet, unsigned setIdx) {
    ew.first = std::rand() % ways;
    ew.second = &cacheSet[ew.first];
}

void RandomPolicy::updateCacheSetReplFields(CacheSet &cacheSet, unsigned int setIdx,
                                            unsigned int wayIdx, bool isHit) {
    // No information needs to be updated
    return;
}

void RandomPolicy::revertCacheSetReplFields(CacheSet &cacheSet,
                                            const CacheWay &oldWay,
                                            unsigned int wayIdx) {
    // No information needs to be updated
    return;
}

/******************************************************************************/
// LRU policy

void LruPolicy::locateEvictionWay(std::pair<unsigned, CacheWay*>& ew,
                                   CacheSet& cacheSet, unsigned setIdx) {
    if (ways == 1) {
        // Nothing to do if we are in LRU and only have 1 way
        ew.first = 0;
        ew.second = &cacheSet[ew.first];
    } else {
        // Check whether each entry is nullptr
        for (int i = 0; i < ways; i++) {
            cacheSet[i];
        }
        // If there is an invalid cache cacheSet, select that
        for (auto& idx_way : cacheSet) {
            if (!idx_way.second.valid) {
                ew.first = idx_way.first;
                ew.second = &idx_way.second;
                break;
            }
        }
        if (ew.second == nullptr) {
            // Else, Find LRU way
            for (auto& idx_way : cacheSet) {
                if (idx_way.second.counter == ways - 1) {
                    ew.first = idx_way.first;
                    ew.second = &idx_way.second;
                    break;
                }
            }
        }
    }
}

void LruPolicy::updateCacheSetReplFields(CacheSet &cacheSet,unsigned int setIdx,
                                         unsigned int wayIdx, bool isHit) {
    const unsigned preLRU = cacheSet[wayIdx].counter;
    for (auto& idx_way : cacheSet) {
        if (idx_way.second.valid && idx_way.second.counter < preLRU) {
            idx_way.second.counter++;
        }
    }
    cacheSet[wayIdx].counter = 0;
}

void LruPolicy::revertCacheSetReplFields(CacheSet &cacheSet,
                                         const CacheWay &oldWay,
                                         unsigned int wayIdx) {
    for (auto& idx_way : cacheSet) {
        if (idx_way.second.valid && idx_way.second.counter <= oldWay.counter) {
            idx_way.second.counter--;
        }
    }
    cacheSet[wayIdx].counter = oldWay.counter;
}

/******************************************************************************/
// LRU-LIP policy

void LruLipPolicy::locateEvictionWay(std::pair<unsigned, CacheWay*>& ew,
                                        CacheSet& cacheSet, unsigned setIdx) {
    // ---------------------Part 2. TODO ------------------------------
}

void LruLipPolicy::updateCacheSetReplFields(CacheSet &cacheSet, unsigned int setIdx,
                                               unsigned int wayIdx, bool isHit) {
    // ---------------------Part 2. TODO ------------------------------
}

void LruLipPolicy::revertCacheSetReplFields(CacheSet &cacheSet,
                                               const CacheWay &oldWay,
                                               unsigned int wayIdx) {
    // ---------------------Part 2. TODO (optional)------------------------------
}

/******************************************************************************/
// DIP policy

void DipPolicy::locateEvictionWay(std::pair<unsigned, CacheWay*>& ew,
                                        CacheSet& cacheSet, unsigned setIdx) {
    // ---------------------Part 2. TODO ------------------------------
}

void DipPolicy::updateCacheSetReplFields(CacheSet &cacheSet, unsigned int setIdx,
                                               unsigned int wayIdx, bool isHit) {
    // ---------------------Part 2. TODO ------------------------------
}

void DipPolicy::revertCacheSetReplFields(CacheSet &cacheSet,
                                               const CacheWay &oldWay,
                                               unsigned int wayIdx) {
    // ---------------------Part 2. TODO (optional)------------------------------
}

/******************************************************************************/
// RRIP policy

void RripPolicy::locateEvictionWay(std::pair<unsigned, CacheWay*>& ew,
                                        CacheSet& cacheSet, unsigned setIdx) {
    // ---------------------Part 2. TODO ------------------------------
}

void RripPolicy::updateCacheSetReplFields(CacheSet &cacheSet, unsigned int setIdx,
                                               unsigned int wayIdx, bool isHit) {
    // ---------------------Part 2. TODO ------------------------------
}

void RripPolicy::revertCacheSetReplFields(CacheSet &cacheSet,
                                               const CacheWay &oldWay,
                                               unsigned int wayIdx) {
    // ---------------------Part 2. TODO (optional) ------------------------------
}

}
