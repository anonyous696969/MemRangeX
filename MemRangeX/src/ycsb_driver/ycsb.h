#ifndef MEMRANGEX_YCSB_H
#define MEMRANGEX_YCSB_H

#include "../common/common_types.h"
#include "../hssi_index/hssi.h"
#include "../cache_system/rl_agent.h"
#include "../numa_concurrency/numa_lock.h"
#include <vector>

class YCSBWorkload
{
public:
    YCSBWorkload(HSSIIndex* hssi, RLCacheAgent* cache, NumaLockManager* lock_mgr);
    void RunAllWorkloads();
    void RunPureQueryWorkload(uint64_t record_cnt);
    void RunReadWriteWorkload(uint64_t record_cnt);
    void RunScanWorkload(uint64_t record_cnt, uint32 scan_len);

private:
    HSSIIndex* hssi_;
    RLCacheAgent* cache_agent_;
    NumaLockManager* lock_mgr_;
    const uint64_t DEFAULT_RECORD = 200000000ULL;
};

#endif