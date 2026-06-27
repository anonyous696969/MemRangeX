#include "rl_agent.h"
#include "../common/utils.h"
#include <algorithm>
#include <limits>

bool RLState::operator<(const RLState& other) const
{
    if (hit_rate != other.hit_rate) return hit_rate < other.hit_rate;
    if (space_used_ratio != other.space_used_ratio) return space_used_ratio < other.space_used_ratio;
    if (data_skew != other.data_skew) return data_skew < other.data_skew;
    if (avg_rdma_delay != other.avg_rdma_delay) return avg_rdma_delay < other.avg_rdma_delay;
    if (lock_conflict_cnt != other.lock_conflict_cnt) return lock_conflict_cnt < other.lock_conflict_cnt;
    return scan_data_ratio < other.scan_data_ratio;
}

RLCacheAgent::RLCacheAgent(L1NumaCache* l1, L2GlobalCache* l2, GlobalConfig* cfg)
    : l1_(l1), l2_(l2), cfg_(cfg)
{
}

float RLCacheAgent::CalcAdmitProb(CacheDataType dtype, float skew, float used_ratio)
{
    float gamma = 1.0f;
    switch (dtype)
    {
        case DATA_INNER_NODE: gamma = cfg_->coeff_inner; break;
        case DATA_LEAF_NODE: gamma = cfg_->coeff_leaf; break;
        case DATA_SCAN_TEMP: gamma = cfg_->coeff_scan; break;
        default: break;
    }
    float p = cfg_->cache_base_prob * (1.0f - used_ratio) * (1.0f + skew) * gamma;
    return std::clamp(p, 0.0f, 1.0f);
}

float RLCacheAgent::CalcReward(float avg_latency, float hit_rate, uint32 rdma_times)
{
    float latency = std::max(avg_latency, std::numeric_limits<float>::epsilon());
    float r1 = cfg_->rl_w1 / latency;
    float r2 = cfg_->rl_w2 * hit_rate;
    float r3 = cfg_->rl_w3 * static_cast<float>(rdma_times);
    return r1 + r2 - r3;
}

void RLCacheAgent::UpdateQTable(const RLState& state, float reward)
{
    float old_q = q_table_[state];
    q_table_[state] = old_q + cfg_->rl_learn_rate * (reward - old_q);
}

bool RLCacheAgent::CacheAccess(KeyType key, ValType& out_val, CacheDataType dtype, float skew)
{
    (void)dtype;
    (void)skew;
    if (l1_->Get(key, out_val))
    {
        return true;
    }
    return l2_->Get(key, out_val);
}

bool RLCacheAgent::Admit(KeyType key, ValType val, CacheDataType dtype, float skew)
{
    uint32 cap = l1_->GetMaxCapacity();
    if (cap == 0)
    {
        return false;
    }
    float used = static_cast<float>(l1_->GetUsedCount()) / static_cast<float>(cap);
    float prob = CalcAdmitProb(dtype, skew, used);
    if (Utils::RandomFloat0To1() < prob)
    {
        return l1_->Put(key, val);
    }
    return false;
}
