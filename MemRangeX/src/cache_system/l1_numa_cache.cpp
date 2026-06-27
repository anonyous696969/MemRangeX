#include "l1_numa_cache.h"
#include "../common/utils.h"

L1NumaCache::L1NumaCache(int numa_id, uint32 capacity)
    : numa_id_(numa_id), cap_(capacity)
{
    Utils::BindThreadToNuma(numa_id_);
}

L1NumaCache::~L1NumaCache() = default;

bool L1NumaCache::Get(KeyType key, ValType& out_val)
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = cache_map_.find(key);
    if (it == cache_map_.end())
    {
        return false;
    }
    out_val = it->second;
    return true;
}

bool L1NumaCache::Put(KeyType key, ValType val)
{
    if (cap_ == 0)
    {
        return false;
    }

    std::lock_guard<std::mutex> lk(mtx_);
    if (cache_map_.find(key) == cache_map_.end() && cache_map_.size() >= cap_)
    {
        EvictOne();
    }
    cache_map_[key] = val;
    return true;
}

uint32 L1NumaCache::GetUsedCount()
{
    std::lock_guard<std::mutex> lk(mtx_);
    return static_cast<uint32>(cache_map_.size());
}

uint32 L1NumaCache::GetMaxCapacity() const
{
    return cap_;
}

void L1NumaCache::EvictOne()
{
    if (!cache_map_.empty())
    {
        cache_map_.erase(cache_map_.begin());
    }
}
