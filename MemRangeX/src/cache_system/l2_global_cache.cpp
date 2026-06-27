#include "l2_global_cache.h"

L2GlobalCache::L2GlobalCache(uint32 capacity)
    : cap_(capacity)
{
}

bool L2GlobalCache::Get(KeyType key, ValType& out_val)
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = cache_.find(key);
    if (it == cache_.end())
    {
        return false;
    }
    out_val = it->second;
    return true;
}

bool L2GlobalCache::Put(KeyType key, ValType val)
{
    if (cap_ == 0)
    {
        return false;
    }

    std::lock_guard<std::mutex> lk(mtx_);
    if (cache_.find(key) == cache_.end() && cache_.size() >= cap_)
    {
        Evict();
    }
    cache_[key] = val;
    return true;
}

uint32 L2GlobalCache::GetUsed()
{
    std::lock_guard<std::mutex> lk(mtx_);
    return static_cast<uint32>(cache_.size());
}

uint32 L2GlobalCache::GetCap() const
{
    return cap_;
}

void L2GlobalCache::Evict()
{
    if (!cache_.empty())
    {
        cache_.erase(cache_.begin());
    }
}
