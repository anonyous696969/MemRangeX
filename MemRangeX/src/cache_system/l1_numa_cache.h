#ifndef MEMRANGEX_L1_NUMA_CACHE_H
#define MEMRANGEX_L1_NUMA_CACHE_H

#include "../common/common_types.h"
#include <mutex>
#include <unordered_map>

class L1NumaCache
{
public:
    L1NumaCache(int numa_id, uint32 capacity);
    ~L1NumaCache();
    bool Get(KeyType key, ValType& out_val);
    bool Put(KeyType key, ValType val);
    uint32 GetUsedCount();
    uint32 GetMaxCapacity() const;
    void EvictOne();

private:
    int numa_id_;
    uint32 cap_;
    std::unordered_map<KeyType, ValType> cache_map_;
    std::mutex mtx_;
};

#endif
