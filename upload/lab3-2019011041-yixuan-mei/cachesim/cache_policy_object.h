#ifndef CACHE_POLICY_OBJECT_H
#define CACHE_POLICY_OBJECT_H

#include "cache_organize_component.h"
#include <iostream>

namespace Ripes {

class CachePolicyBase
{
public:
    CachePolicyBase(int number_ways, int number_sets, int number_blocks): ways(number_ways), sets(number_sets), blocks(number_blocks) {}
    virtual void locateEvictionWay(std::pair<unsigned, CacheWay*>& ew, CacheSet& cacheSet, unsigned setIdx) = 0;
    virtual void updateCacheSetReplFields(CacheSet& cacheSet, unsigned int setIdx, unsigned wayIdx, bool isHit) = 0;
    virtual void revertCacheSetReplFields(CacheSet& cacheSet, const CacheWay& oldWay, unsigned wayIdx) = 0;
    virtual ~CachePolicyBase() {}
protected:
    int ways;
    int sets;
    int blocks;
};

class RandomPolicy : public CachePolicyBase {
public:
    RandomPolicy(int number_ways, int number_sets, int number_blocks) : CachePolicyBase(number_ways, number_sets, number_blocks) {}
    void locateEvictionWay(std::pair<unsigned, CacheWay*>& ew, CacheSet& cacheSet, unsigned setIdx) override;
    void updateCacheSetReplFields(CacheSet& cacheSet, unsigned int setIdx, unsigned wayIdx, bool isHit) override;
    void revertCacheSetReplFields(CacheSet& cacheSet, const CacheWay& oldWay, unsigned wayIdx) override;
    ~RandomPolicy() {}
};


class LruPolicy : public CachePolicyBase {
public:
    LruPolicy(int number_ways, int number_sets, int number_blocks) : CachePolicyBase(number_ways, number_sets, number_blocks) {}
    void locateEvictionWay(std::pair<unsigned, CacheWay*>& ew, CacheSet& cacheSet, unsigned setIdx) override;
    void updateCacheSetReplFields(CacheSet& cacheSet, unsigned int setIdx, unsigned wayIdx, bool isHit) override;
    void revertCacheSetReplFields(CacheSet& cacheSet, const CacheWay& oldWay, unsigned wayIdx) override;
    ~LruPolicy() {}
};

class LruLipPolicy : public CachePolicyBase {
public:
    LruLipPolicy(int number_ways ,int number_sets, int number_blocks) :
        CachePolicyBase(number_ways, number_sets, number_blocks){}
    void locateEvictionWay(std::pair<unsigned, CacheWay*>& ew, CacheSet& cacheSet, unsigned setIdx) override;
    void updateCacheSetReplFields(CacheSet& cacheSet, unsigned int setIdx, unsigned wayIdx, bool isHit) override;
    void revertCacheSetReplFields(CacheSet& cacheSet, const CacheWay& oldWay, unsigned wayIdx) override;
    ~LruLipPolicy() {}
};

class DipPolicy : public CachePolicyBase {
public:
    DipPolicy(int number_ways, int number_sets, int number_blocks) : CachePolicyBase(number_ways, number_sets, number_blocks) {}
    void locateEvictionWay(std::pair<unsigned, CacheWay*>& ew, CacheSet& cacheSet, unsigned setIdx) override;
    void updateCacheSetReplFields(CacheSet& cacheSet, unsigned int setIdx, unsigned wayIdx, bool isHit) override;
    void revertCacheSetReplFields(CacheSet& cacheSet, const CacheWay& oldWay, unsigned wayIdx) override;
    ~DipPolicy() {}
};


class RripPolicy : public CachePolicyBase {
public:
    RripPolicy(int number_ways, int number_sets, int number_blocks) : CachePolicyBase(number_ways, number_sets, number_blocks) {}
    void locateEvictionWay(std::pair<unsigned, CacheWay*>& ew, CacheSet& cacheSet, unsigned setIdx) override;
    void updateCacheSetReplFields(CacheSet& cacheSet, unsigned int setIdx, unsigned wayIdx, bool isHit) override;
    void revertCacheSetReplFields(CacheSet& cacheSet, const CacheWay& oldWay, unsigned wayIdx) override;
    ~RripPolicy() {}
};


}

#endif // CACHE_POLICY_OBJECT_H
