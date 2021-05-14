#include "cache_policy_object.h"
#include "cache_organize_component.h"
#include <iostream>
#include <assert.h>
#include <cmath>

#define __DEBUG__

using namespace std;

// DIP parameters
constexpr unsigned RESETTHRESHOLD = 100 * 1000;
enum class InsertPolicy { MIP = 0, LIP = 1 };

// DIP used counters
InsertPolicy insert_policy = InsertPolicy::LIP;
unsigned total_mem_access = 0;
unsigned mip_miss = 0;
unsigned mip_hit = 0;
unsigned lip_miss = 0;
unsigned lip_hit = 0;

// RRIP parameters
constexpr unsigned RRIBITWIDTH = 3;
constexpr unsigned DISTANTRRI = 7;
constexpr unsigned LONGRRI = 6;

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
    // This is identical to normal LRU
    if (ways == 1) {
        // Nothing to do if we are in LRU and only have 1 way
        ew.first = 0;
        ew.second = &cacheSet[ew.first];
    }
    else {
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

void LruLipPolicy::updateCacheSetReplFields(CacheSet &cacheSet, unsigned int setIdx,
                                               unsigned int wayIdx, bool isHit) {
    // ---------------------Part 2. TODO ------------------------------
    
    // Get counter of current entry
    const unsigned preLRU = cacheSet[wayIdx].counter;
    
    // If this is an insertion, apply LIP
    if (preLRU == (unsigned)-1) {

#ifdef __DEBUG__

        cout << "setIdx = " << setIdx << endl;

#endif // __DEBUG__
        
        // Calculate current max counter and use it to ensure consistency of
        // counters. Apply to new entry counter and exit.
        cacheSet[wayIdx].counter = 0;
        for (auto& idx_way : cacheSet) {

            if (idx_way.second.valid && idx_way.second.counter >= cacheSet[wayIdx].counter
                && idx_way.first != wayIdx) {
                cacheSet[wayIdx].counter = idx_way.second.counter + 1;
            }

#ifdef __DEBUG__

            cout << "idx_way counter = " << idx_way.second.counter << ", valid = "
                << idx_way.second.valid << endl;

#endif // __DEBUG__

        }

#ifdef __DEBUG__

        cout << "newly insert counter = " << cacheSet[wayIdx].counter << endl;

#endif // __DEBUG__

        return;
    }
    
    // If this is a normal access, apply LRU
    for (auto& idx_way : cacheSet) {
        if (idx_way.second.valid && idx_way.second.counter < preLRU) {
            idx_way.second.counter++;
        }
    }
    cacheSet[wayIdx].counter = 0;
}

void LruLipPolicy::revertCacheSetReplFields(CacheSet &cacheSet,
                                               const CacheWay &oldWay,
                                               unsigned int wayIdx) {
    // ---------------------Part 2. TODO (optional)------------------------------
    // This is only called when reverting upon hit (access). Therefore, this is also
    // identical to LRU
    // Potential bug: influence of insert on other counters not nullified!
    for (auto& idx_way : cacheSet) {
        if (idx_way.second.valid && idx_way.second.counter <= oldWay.counter) {
            idx_way.second.counter--;
        }
    }
    cacheSet[wayIdx].counter = oldWay.counter;
}

/******************************************************************************/
// DIP policy

void DipPolicy::locateEvictionWay(std::pair<unsigned, CacheWay*>& ew,
                                        CacheSet& cacheSet, unsigned setIdx) {
    // ---------------------Part 2. TODO ------------------------------
    // This is identical to normal LRU
    if (ways == 1) {
        // Nothing to do if we are in LRU and only have 1 way
        ew.first = 0;
        ew.second = &cacheSet[ew.first];
    }
    else {
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

void DipPolicy::updateCacheSetReplFields(CacheSet &cacheSet, unsigned int setIdx,
                                               unsigned int wayIdx, bool isHit) {
    // ---------------------Part 2. TODO ------------------------------
    // Implement the simplest case of DIP, i.e. use SET0 as MIP sample,
    // SET1 as LIP sample

    // Update counters
    total_mem_access += 1;
    if (setIdx == 0) {

        // SET0, MIP
        mip_miss += (isHit ? 0 : 1);
        mip_hit += (isHit ? 1 : 0);

    } else if (setIdx == 1) {

        // SET1, LIP
        lip_miss += (isHit ? 0 : 1);
        lip_hit += (isHit ? 1 : 0);

    }

    // Calculate hitrate and update policy
    float mip_hit_rate = (mip_hit + mip_miss) > 0 ?
        static_cast<float>(mip_hit) / mip_hit + mip_miss : 0;
    float lip_hit_rate = (lip_hit + lip_miss) > 0 ?
        static_cast<float>(lip_hit) / lip_hit + lip_miss : 0;
    insert_policy = (mip_hit_rate >= lip_hit_rate ?
        InsertPolicy::MIP : InsertPolicy::LIP);

    // Reset counter
    if (total_mem_access == RESETTHRESHOLD) {

        total_mem_access = 0;
        mip_miss = 0;
        mip_hit = 0;
        lip_miss = 0;
        lip_hit = 0;

    }

    // Update cache
    // SET0: MIP, SET1:LIP, other: insert_policy
    if (setIdx == 0 || (setIdx != 1 && insert_policy == InsertPolicy::MIP)) {

        // SET0 or MIP is current policy
        const unsigned preLRU = cacheSet[wayIdx].counter;
        for (auto& idx_way : cacheSet) {
            if (idx_way.second.valid && idx_way.second.counter < preLRU) {
                idx_way.second.counter++;
            }
        }
        cacheSet[wayIdx].counter = 0;

    } 
    else if (setIdx == 1 || (setIdx != 0 && insert_policy == InsertPolicy::LIP)) {

        // SET1 or LIP is current policy
        // Get counter of current entry
        const unsigned preLRU = cacheSet[wayIdx].counter;

        // If this is an insertion, apply LIP
        if (preLRU == (unsigned)-1) {

#ifdef __DEBUG__

            cout << "setIdx = " << setIdx << endl;

#endif // __DEBUG__

            // Calculate current max counter and use it to ensure consistency of
            // counters. Apply to new entry counter and exit.
            cacheSet[wayIdx].counter = 0;
            for (auto& idx_way : cacheSet) {

                if (idx_way.second.valid && idx_way.second.counter >= cacheSet[wayIdx].counter
                    && idx_way.first != wayIdx) {
                    cacheSet[wayIdx].counter = idx_way.second.counter + 1;
                }

#ifdef __DEBUG__

                cout << "idx_way counter = " << idx_way.second.counter << ", valid = "
                    << idx_way.second.valid << endl;

#endif // __DEBUG__

            }

#ifdef __DEBUG__

            cout << "newly insert counter = " << cacheSet[wayIdx].counter << endl;

#endif // __DEBUG__

            return;
        }

        // If this is a normal access, apply LRU
        for (auto& idx_way : cacheSet) {
            if (idx_way.second.valid && idx_way.second.counter < preLRU) {
                idx_way.second.counter++;
            }
        }
        cacheSet[wayIdx].counter = 0;


    } 
    else {

        // Fall through
        cerr << "Cache policy selection failure!" << endl;
        assert(false);

    }

}

void DipPolicy::revertCacheSetReplFields(CacheSet &cacheSet,
                                               const CacheWay &oldWay,
                                               unsigned int wayIdx) {
    // ---------------------Part 2. TODO (optional)------------------------------
    // This is only called when reverting upon hit (access). Therefore, this is also
    // identical to LRU
    // Potential bug: influence of insert on other counters not nullified!
    for (auto& idx_way : cacheSet) {
        if (idx_way.second.valid && idx_way.second.counter <= oldWay.counter) {
            idx_way.second.counter--;
        }
    }
    cacheSet[wayIdx].counter = oldWay.counter;
}

/******************************************************************************/
// RRIP policy

void RripPolicy::locateEvictionWay(std::pair<unsigned, CacheWay*>& ew,
                                        CacheSet& cacheSet, unsigned setIdx) {
    // ---------------------Part 2. TODO ------------------------------
    // Use counter field to store RRI of each cacheway

    if (ways == 1) {

        // 1-way cache, only one choice
        ew.first = 0;
        ew.second = &cacheSet[ew.first];
    
    }
    else {

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

        // Apply RRIP if all ways are occpuied
        if (ew.second == nullptr) {

#ifdef __DEBUG__

            cout << "SetID = " << setIdx << endl;

#endif // __DEBUG__

            // Find entry with largest RRI
            unsigned max_RRI = 0;
            for (auto& idx_way : cacheSet) {
                if (idx_way.second.counter >= max_RRI) {
                    ew.first = idx_way.first;
                    ew.second = &idx_way.second;
                    max_RRI = idx_way.second.counter;
                }

#ifdef __DEBUG__

                cout << "idx_way RRI = " << idx_way.second.counter << endl;

#endif // __DEBUG__

            }

#ifdef __DEBUG__

            cout << "Selected way = " << ew.first << endl;

#endif // __DEBUG__

            // Normalize to DISTANTRRI
            if (max_RRI == DISTANTRRI) { return; }
            else {
                unsigned delta = DISTANTRRI - max_RRI;
                assert(delta > 0);
                for (auto& idx_way : cacheSet) {
                    idx_way.second.counter += delta;
                }
            }

        }

    }

}

void RripPolicy::updateCacheSetReplFields(CacheSet &cacheSet, unsigned int setIdx,
                                               unsigned int wayIdx, bool isHit) {
    // ---------------------Part 2. TODO ------------------------------
    // Use counter field to store RRI of each cacheway

    // Update cacheway RRI upon access (hit) and insert
    if (isHit) cacheSet[wayIdx].counter = 0;
    else cacheSet[wayIdx].counter = LONGRRI;

}

void RripPolicy::revertCacheSetReplFields(CacheSet &cacheSet,
                                               const CacheWay &oldWay,
                                               unsigned int wayIdx) {
    // ---------------------Part 2. TODO (optional) ------------------------------
    // Not implemented
    assert(false);

}

}
