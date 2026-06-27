#ifndef MEMRANGEX_NUMA_LOCK_H
#define MEMRANGEX_NUMA_LOCK_H

#include "../common/common_types.h"
#include <vector>
#include <pthread.h>
#include <immintrin.h>


class NumaLockManager
{
public:
    NumaLockManager(int numa_id, uint32 sub_block_num);
    ~NumaLockManager();

    
    bool ReadLock(uint32 page_id, uint64_t& out_version);
    void ReadUnlock(uint32 page_id);

    
    bool WriteLock(uint32 page_id, uint32 sub_id);
    void WriteUnlock(uint32 page_id, uint32 sub_id);

    
    bool HtmTransactionUpdate(KeyType key, ValType& val);

    
    double CalcConflictProb(uint32 thread_num, uint32 m_sub_block);

private:
    int numa_node_id_;
    uint32 sub_block_count_;
    std::vector<std::vector<pthread_mutex_t>> sub_block_locks_;
    std::vector<uint64_t> page_version_;
    const uint32 MAX_PAGE_NUM = 1024;
};

#endif