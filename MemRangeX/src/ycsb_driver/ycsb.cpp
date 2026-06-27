#include "ycsb.h"
#include "../common/utils.h"

YCSBWorkload::YCSBWorkload(HSSIIndex* hssi, RLCacheAgent* cache, NumaLockManager* lock_mgr)
    : hssi_(hssi), cache_agent_(cache), lock_mgr_(lock_mgr)
{
}

void YCSBWorkload::RunAllWorkloads()
{
    RunPureQueryWorkload(DEFAULT_RECORD / 100);
    RunReadWriteWorkload(DEFAULT_RECORD / 100);
    RunScanWorkload(DEFAULT_RECORD / 100, 100);
}

void YCSBWorkload::RunPureQueryWorkload(uint64_t record_cnt)
{
    ValType val_out = 0;
    for (uint64 i = 0; i < record_cnt; ++i)
    {
        KeyType k = static_cast<KeyType>(Utils::RandomFloat0To1() * static_cast<float>(DEFAULT_RECORD));
        cache_agent_->CacheAccess(k, val_out, DATA_LEAF_NODE, 0.99f);
    }
}

void YCSBWorkload::RunReadWriteWorkload(uint64_t record_cnt)
{
    ValType val = 0;
    for (uint64 i = 0; i < record_cnt; ++i)
    {
        KeyType k = static_cast<KeyType>(Utils::RandomFloat0To1() * static_cast<float>(DEFAULT_RECORD));
        if (i % 20 == 0)
        {
            hssi_->Insert(k, i);
        }
        else if (hssi_->PointQuery(k, val))
        {
            cache_agent_->Admit(k, val, DATA_LEAF_NODE, 0.99f);
        }
    }
}

void YCSBWorkload::RunScanWorkload(uint64_t record_cnt, uint32 scan_len)
{
    std::vector<std::pair<KeyType, ValType>> res;
    for (uint64 i = 0; i < record_cnt; ++i)
    {
        KeyType l = static_cast<KeyType>(Utils::RandomFloat0To1() * static_cast<float>(DEFAULT_RECORD));
        KeyType r = l + scan_len;
        hssi_->RangeScan(l, r, res);
    }
}
