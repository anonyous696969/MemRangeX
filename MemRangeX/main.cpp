#include "common/common_types.h"
#include "common/config_loader.h"
#include "common/logger.h"
#include "common/utils.h"
#include "rdma/rd_context.h"
#include "rdma/rd_server.h"
#include "numa_concurrency/numa_lock.h"
#include "hssi_index/hssi.h"
#include "load_collector/load_monitor.h"
#include "offload_engine/offload.h"
#include "cache_system/l1_numa_cache.h"
#include "cache_system/l2_global_cache.h"
#include "cache_system/rl_agent.h"
#include "segment_manager/seg_mgr.h"
#include "fault_tolerance/fault.h"
#include "metadata/meta.h"
#include "thread_pool/thread_pool.h"
#include "ycsb_driver/ycsb.h"
#include <atomic>
#include <csignal>
#include <iostream>

namespace
{
std::atomic<bool> g_running(true);

void HandleSignal(int signal)
{
    (void)signal;
    g_running = false;
}
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    GlobalConfig global_cfg;
    bool cfg_load_ok = ConfigLoader::LoadConfig("config.ini", global_cfg);
    if (!cfg_load_ok)
    {
        std::cerr << "Load config failed, exit program" << std::endl;
        return -1;
    }

    Logger::GetInstance().Init(global_cfg.log_path, global_cfg.log_level);
    LOG_INFO_STR("========== MemRangeX Startup ==========");

    RdContext rd_ctx;
    if (!rd_ctx.Init(global_cfg.rdma_port, global_cfg.node_role))
    {
        LOG_ERROR_STR("RDMA context init failed");
        return -1;
    }

    RdServer rd_server(&rd_ctx, &global_cfg);
    rd_server.Start();

    HSSIIndex hssi(global_cfg.seg_max_cap, global_cfg.seg_min_cap, global_cfg.hot_threshold);
    LoadMonitor load_mon(&global_cfg);
    load_mon.StartMonitor(global_cfg.collect_interval_ms);

    NumaLockManager numa_lock(0, global_cfg.lock_sub_block);
    OffloadEngine off_engine(&hssi, &load_mon, global_cfg.beta_max_limit);
    L1NumaCache l1_cache(0, global_cfg.l1_cache_size);
    L2GlobalCache l2_cache(global_cfg.l2_cache_size);
    RLCacheAgent rl_cache(&l1_cache, &l2_cache, &global_cfg);
    SegmentManager seg_mgr(&hssi, &global_cfg);
    FaultHandler fault_hdl;
    MetaNode meta_node(&global_cfg);
    ThreadPool worker_pool(global_cfg.worker_threads);
    YCSBWorkload ycsb_driver(&hssi, &rl_cache, &numa_lock);

    (void)off_engine;
    (void)fault_hdl;

    seg_mgr.StartMonitor();
    worker_pool.SubmitTask([&]() {
        LOG_INFO_STR("Start execute full YCSB benchmark workload");
        ycsb_driver.RunAllWorkloads();
        LOG_INFO_STR("All YCSB benchmark finished");
    });

    LOG_INFO_STR("All core modules initialized successfully, enter main blocking loop");

    while (g_running.load())
    {
        LoadInfo local_load = load_mon.GetLocalLoad();
        meta_node.UpdateNodeLoad(global_cfg.node_id, local_load);
        auto global_load_view = meta_node.GetGlobalLoadView();
        load_mon.SyncGlobalLoadView(global_load_view);
        Utils::SleepMs(1000);
    }

    worker_pool.Stop();
    seg_mgr.StopMonitor();
    load_mon.StopMonitor();
    rd_server.Stop();
    LOG_INFO_STR("MemRangeX shutdown complete");
    return 0;
}
