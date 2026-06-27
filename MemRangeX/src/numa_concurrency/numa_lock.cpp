#include "numa_lock.h"
#include "../common/logger.h"
#include "../common/utils.h"
#include <cmath>

NumaLockManager::NumaLockManager(int numa_id, uint32 sub_block_num)
    : numa_node_id_(numa_id), sub_block_count_(sub_block_num == 0 ? 1 : sub_block_num)
{
    Utils::BindThreadToNuma(numa_node_id_);
    sub_block_locks_.resize(MAX_PAGE_NUM);
    page_version_.resize(MAX_PAGE_NUM, 1);
    for (uint32 i = 0; i < MAX_PAGE_NUM; ++i)
    {
        sub_block_locks_[i].resize(sub_block_count_);
        for (uint32 j = 0; j < sub_block_count_; ++j)
        {
            pthread_mutex_init(&sub_block_locks_[i][j], nullptr);
        }
    }
    LOG_INFO_STR("NUMA lock manager init done, numa_id=" + std::to_string(numa_id) + ", sub_block=" + std::to_string(sub_block_count_));
}

NumaLockManager::~NumaLockManager()
{
    for (uint32 i = 0; i < MAX_PAGE_NUM; ++i)
    {
        for (uint32 j = 0; j < sub_block_count_; ++j)
        {
            pthread_mutex_destroy(&sub_block_locks_[i][j]);
        }
    }
}

bool NumaLockManager::ReadLock(uint32 page_id, uint64_t& out_version)
{
    if (page_id >= MAX_PAGE_NUM)
    {
        LOG_ERROR_STR("ReadLock invalid page id: " + std::to_string(page_id));
        return false;
    }
    out_version = page_version_[page_id];
    return true;
}

void NumaLockManager::ReadUnlock(uint32 page_id)
{
    (void)page_id;
}

bool NumaLockManager::WriteLock(uint32 page_id, uint32 sub_id)
{
    if (page_id >= MAX_PAGE_NUM || sub_id >= sub_block_count_)
    {
        LOG_ERROR_STR("WriteLock invalid page/sub id");
        return false;
    }
    return pthread_mutex_lock(&sub_block_locks_[page_id][sub_id]) == 0;
}

void NumaLockManager::WriteUnlock(uint32 page_id, uint32 sub_id)
{
    if (page_id >= MAX_PAGE_NUM || sub_id >= sub_block_count_)
    {
        return;
    }
    pthread_mutex_unlock(&sub_block_locks_[page_id][sub_id]);
    ++page_version_[page_id];
}

bool NumaLockManager::HtmTransactionUpdate(KeyType key, ValType& val)
{
    (void)key;
#if defined(__RTM__)
    unsigned status = _xbegin();
    if (status == _XBEGIN_STARTED)
    {
        val += 1;
        _xend();
        return true;
    }
    LOG_DEBUG_STR("HTM transaction abort, fallback to software lock");
#else
    (void)val;
#endif
    return false;
}

double NumaLockManager::CalcConflictProb(uint32 thread_num, uint32 m_sub_block)
{
    if (m_sub_block == 0)
    {
        return 1.0;
    }
    if (thread_num == 0 || thread_num == 1)
    {
        return 0.0;
    }
    if (thread_num > m_sub_block)
    {
        return 1.0;
    }

    double numerator = 1.0;
    double denominator = std::pow(static_cast<double>(m_sub_block), static_cast<double>(thread_num));
    for (uint32 i = 0; i < thread_num; ++i)
    {
        numerator *= static_cast<double>(m_sub_block - i);
    }
    double no_conflict = numerator / denominator;
    return 1.0 - no_conflict;
}
