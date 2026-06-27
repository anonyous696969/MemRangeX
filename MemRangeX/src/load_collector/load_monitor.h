#ifndef MEMRANGEX_LOAD_MONITOR_H
#define MEMRANGEX_LOAD_MONITOR_H

#include "../common/common_types.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>

class LoadMonitor
{
public:
    explicit LoadMonitor(GlobalConfig* cfg);
    ~LoadMonitor();
    void StartMonitor(TimeMs interval_ms);
    void StopMonitor();
    LoadInfo GetLocalLoad();
    void SyncGlobalLoadView(const std::unordered_map<uint32_t, LoadInfo>& global_view);
    std::unordered_map<uint32_t, LoadInfo> GetGlobalLoadView();

private:
    void CollectLoop();
    void CalculateLoadBeta(LoadInfo& load);
    GlobalConfig* cfg_;
    std::atomic<bool> running_;
    TimeMs collect_interval_;
    LoadInfo local_load_;
    std::mutex local_load_mtx_;
    std::unordered_map<uint32_t, LoadInfo> global_load_view_;
    std::mutex global_view_mtx_;
    std::thread collect_thread_;
};

#endif
