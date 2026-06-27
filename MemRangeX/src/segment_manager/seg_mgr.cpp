#include "seg_mgr.h"
#include "../common/logger.h"
#include "../common/utils.h"

SegmentManager::SegmentManager(HSSIIndex* hssi, GlobalConfig* cfg)
    : hssi_(hssi), cfg_(cfg), running_(false)
{
}

SegmentManager::~SegmentManager()
{
    StopMonitor();
}

void SegmentManager::StartMonitor()
{
    if (running_.load())
    {
        LOG_WARN_STR("Segment monitor already running");
        return;
    }
    running_ = true;
    monitor_thread_ = std::thread(&SegmentManager::MonitorLoop, this);
    LOG_INFO_STR("Segment split/merge monitor started");
}

void SegmentManager::StopMonitor()
{
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false) && !monitor_thread_.joinable())
    {
        return;
    }
    if (monitor_thread_.joinable())
    {
        monitor_thread_.join();
    }
    LOG_INFO_STR("Segment monitor stopped");
}

void SegmentManager::MonitorLoop()
{
    while (running_.load())
    {
        CheckSplitMergeRule();
        Utils::SleepMs(500);
    }
}

void SegmentManager::CheckSplitMergeRule()
{
    uint32 i = 0;
    while (i < hssi_->GetSegmentCount())
    {
        Segment* s = hssi_->GetSegmentById(i);
        if (s == nullptr)
        {
            ++i;
            continue;
        }

        bool need_split = (s->total_data_num > cfg_->seg_max_cap) || (s->access_hot_count > cfg_->hot_threshold);
        if (need_split)
        {
            hssi_->SplitSegment(i);
            ++i;
            continue;
        }

        if (i + 1 >= hssi_->GetSegmentCount())
        {
            ++i;
            continue;
        }

        Segment* s_next = hssi_->GetSegmentById(i + 1);
        if (s_next == nullptr)
        {
            ++i;
            continue;
        }

        bool can_merge = (s->total_data_num < cfg_->seg_min_cap) &&
                         (s_next->total_data_num < cfg_->seg_min_cap) &&
                         (!s->is_hot_segment) &&
                         (!s_next->is_hot_segment);
        if (can_merge)
        {
            hssi_->MergeSegment(i, i + 1);
            continue;
        }
        ++i;
    }
}
