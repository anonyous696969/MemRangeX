#ifndef MEMRANGEX_RL_AGENT_H
#define MEMRANGEX_RL_AGENT_H

#include "../common/common_types.h"
#include "l1_numa_cache.h"
#include "l2_global_cache.h"
#include <map>

enum CacheDataType
{
    DATA_INNER_NODE,
    DATA_LEAF_NODE,
    DATA_SCAN_TEMP
};

struct RLState
{
    float hit_rate;
    float space_used_ratio;
    float data_skew;
    TimeNs avg_rdma_delay;
    uint32 lock_conflict_cnt;
    float scan_data_ratio;

    bool operator<(const RLState& other) const;
};

class RLCacheAgent
{
public:
    RLCacheAgent(L1NumaCache* l1, L2GlobalCache* l2, GlobalConfig* cfg);
    float CalcAdmitProb(CacheDataType dtype, float skew, float used_ratio);
    float CalcReward(float avg_latency, float hit_rate, uint32 rdma_times);
    void UpdateQTable(const RLState& state, float reward);
    bool CacheAccess(KeyType key, ValType& out_val, CacheDataType dtype, float skew);
    bool Admit(KeyType key, ValType val, CacheDataType dtype, float skew);

private:
    L1NumaCache* l1_;
    L2GlobalCache* l2_;
    GlobalConfig* cfg_;
    std::map<RLState, float> q_table_;
};

#endif
