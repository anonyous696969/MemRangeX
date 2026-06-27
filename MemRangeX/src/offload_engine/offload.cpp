#include "offload.h"

OffloadEngine::OffloadEngine(HSSIIndex* hssi, LoadMonitor* monitor, float beta_max_limit)
    : hssi_(hssi), load_monitor_(monitor), beta_max_(beta_max_limit)
{
}

TimeNs OffloadEngine::CalcCostRemoteFetch(const OffloadCostParam& param)
{
    uint64 inner = param.t_r + static_cast<uint64>(param.alpha * static_cast<float>(param.t_l));
    return param.h_depth * inner;
}

TimeNs OffloadEngine::CalcCostPointOffload(const OffloadCostParam& param, float beta)
{
    uint64 inner = static_cast<uint64>(beta * static_cast<float>(param.h_depth * param.t_l));
    return param.t_rpc + inner;
}

TimeNs OffloadEngine::CalcCostScanOffload(const OffloadCostParam& param, float beta, uint32 scan_k)
{
    uint64 inner = param.h_depth * param.t_l + scan_k * param.t_scan_single;
    uint64 beta_part = static_cast<uint64>(beta * static_cast<float>(inner));
    return param.t_rpc + beta_part;
}

bool OffloadEngine::ShouldOffload(const OffloadCostParam& param, float beta)
{
    if (IsOverLoadLimit(beta))
    {
        return false;
    }
    TimeNs cost_r = CalcCostRemoteFetch(param);
    TimeNs cost_o = CalcCostPointOffload(param, beta);
    return cost_o < cost_r;
}

bool OffloadEngine::IsOverLoadLimit(float beta)
{
    return beta >= beta_max_;
}
