#ifndef MEMRANGEX_SEG_MGR_H
#define MEMRANGEX_SEG_MGR_H

#include "../hssi_index/hssi.h"
#include "../common/common_types.h"
#include <thread>
#include <atomic>

class SegmentManager
{
public:
    SegmentManager(HSSIIndex* hssi, GlobalConfig* cfg);
    ~SegmentManager();
    void StartMonitor();
    void StopMonitor();

private:
    void MonitorLoop();
    void CheckSplitMergeRule();
    HSSIIndex* hssi_;
    GlobalConfig* cfg_;
    std::atomic<bool> running_;
    std::thread monitor_thread_;
};

#endif