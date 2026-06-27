#include "load_monitor.h"
#include "../common/logger.h"
#include "../common/utils.h"

LoadMonitor::LoadMonitor(GlobalConfig* cfg)
    : cfg_(cfg), running_(false), collect_interval_(10)
{
}

LoadMonitor::~LoadMonitor()
{
    StopMonitor();
}

void LoadMonitor::StartMonitor(TimeMs interval_ms)
{
    if (running_.load())
    {
        LOG_WARN_STR("Load monitor already running");
        return;
    }
    collect_interval_ = interval_ms == 0 ? 10 : interval_ms;
    running_ = true;
    collect_thread_ = std::thread(&LoadMonitor::CollectLoop, this);
    LOG_INFO_STR("Load monitor started, interval=" + std::to_string(collect_interval_) + "ms");
}

void LoadMonitor::StopMonitor()
{
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false) && !collect_thread_.joinable())
    {
        return;
    }
    if (collect_thread_.joinable())
    {
        collect_thread_.join();
    }
    LOG_INFO_STR("Load monitor stopped");
}

void LoadMonitor::CollectLoop()
{
    while (running_.load())
    {
        LoadInfo next_load;
        next_load.cpu_util = Utils::RandomFloat0To1();
        next_load.task_queue_len = static_cast<uint32>(Utils::RandomFloat0To1() * 1024.0f);
        next_load.rdma_bandwidth_util = Utils::RandomFloat0To1();
        CalculateLoadBeta(next_load);
        {
            std::lock_guard<std::mutex> lk(local_load_mtx_);
            local_load_ = next_load;
        }
        Utils::SleepMs(collect_interval_);
    }
}

void LoadMonitor::CalculateLoadBeta(LoadInfo& load)
{
    float beta = cfg_->beta_low;
    if (load.cpu_util > 0.7f || load.task_queue_len > 512)
    {
        beta = cfg_->beta_high;
        load.level = LOAD_LEVEL_HIGH;
    }
    else if (load.cpu_util > 0.3f)
    {
        beta = cfg_->beta_mid;
        load.level = LOAD_LEVEL_MID;
    }
    else
    {
        load.level = LOAD_LEVEL_LOW;
    }
    load.load_beta = beta;
}

LoadInfo LoadMonitor::GetLocalLoad()
{
    std::lock_guard<std::mutex> lk(local_load_mtx_);
    return local_load_;
}

void LoadMonitor::SyncGlobalLoadView(const std::unordered_map<uint32_t, LoadInfo>& global_view)
{
    std::lock_guard<std::mutex> lk(global_view_mtx_);
    global_load_view_ = global_view;
}

std::unordered_map<uint32_t, LoadInfo> LoadMonitor::GetGlobalLoadView()
{
    std::lock_guard<std::mutex> lk(global_view_mtx_);
    return global_load_view_;
}
