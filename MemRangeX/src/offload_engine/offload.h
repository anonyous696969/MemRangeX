#ifndef MEMRANGEX_OFFLOAD_H
#define MEMRANGEX_OFFLOAD_H

#include "../common/common_types.h"
#include "../hssi_index/hssi.h"
#include "../load_collector/load_monitor.h"


struct OffloadCostParam
{
    uint32 h_depth;
    TimeNs t_r;
    TimeNs t_l;
    float alpha;
    TimeNs t_rpc;
    TimeNs t_scan_single;
};

class OffloadEngine
{
public:
    OffloadEngine(HSSIIndex* hssi, LoadMonitor* monitor, float beta_max_limit);
    
    TimeNs CalcCostRemoteFetch(const OffloadCostParam& param);
    
    TimeNs CalcCostPointOffload(const OffloadCostParam& param, float beta);
    
    TimeNs CalcCostScanOffload(const OffloadCostParam& param, float beta, uint32 scan_k);
    
    bool ShouldOffload(const OffloadCostParam& param, float beta);
    
    bool IsOverLoadLimit(float beta);

private:
    HSSIIndex* hssi_;
    LoadMonitor* load_monitor_;
    float beta_max_;
};

#endif