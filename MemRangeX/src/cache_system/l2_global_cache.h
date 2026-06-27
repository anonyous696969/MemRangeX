#ifndef MEMRANGEX_L2_GLOBAL_CACHE_H
#define MEMRANGEX_L2_GLOBAL_CACHE_H

#include "../common/common_types.h"
#include <mutex>
#include <unordered_map>

class L2GlobalCache
{
public:
    explicit L2GlobalCache(uint32 capacity);
    bool Get(KeyType key, ValType& out_val);
    bool Put(KeyType key, ValType val);
    uint32 GetUsed();
    uint32 GetCap() const;
    void Evict();

private:
    uint32 cap_;
    std::unordered_map<KeyType, ValType> cache_;
    std::mutex mtx_;
};

#endif
