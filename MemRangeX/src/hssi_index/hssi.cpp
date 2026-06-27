#include "hssi.h"
#include "../common/logger.h"
#include "../common/utils.h"
#include <algorithm>
#include <limits>

HSSIIndex::HSSIIndex(uint32 seg_max, uint32 seg_min, uint32 hot_thresh)
    : next_seg_id_(1), seg_max_cap_(seg_max), seg_min_cap_(seg_min), hot_threshold_(hot_thresh)
{
    seg_list_.emplace_back();
    Segment& first_seg = seg_list_.front();
    first_seg.seg_start_key = 0;
    first_seg.seg_end_key = std::numeric_limits<KeyType>::max();
    LOG_INFO_STR("HSSI index initialized, max_cap=" + std::to_string(seg_max_cap_) + ", hot_thresh=" + std::to_string(hot_threshold_));
}

HSSIIndex::~HSSIIndex() = default;

uint32 HSSIIndex::LocateSegId(KeyType key)
{
    for (uint32 i = 0; i < seg_list_.size(); ++i)
    {
        Segment& s = seg_list_[i];
        if (key >= s.seg_start_key && key <= s.seg_end_key)
        {
            return i;
        }
    }
    return 0;
}

Segment* HSSIIndex::GetSegmentById(uint32 seg_id)
{
    if (seg_id >= seg_list_.size())
    {
        return nullptr;
    }
    return &seg_list_[seg_id];
}

size_t HSSIIndex::GetSegmentCount()
{
    std::lock_guard<std::mutex> lk(global_mtx_);
    return seg_list_.size();
}

bool HSSIIndex::Insert(KeyType key, ValType val)
{
    std::lock_guard<std::mutex> lk(global_mtx_);
    auto route_it = key_to_seg_.find(key);
    if (route_it != key_to_seg_.end() && route_it->second < seg_list_.size())
    {
        Segment& seg = seg_list_[route_it->second];
        pthread_rwlock_wrlock(&seg.seg_rw_lock);
        for (auto& pair : seg.data_list)
        {
            if (pair.first == key)
            {
                pair.second = val;
                pthread_rwlock_unlock(&seg.seg_rw_lock);
                UpdateHotStat(key);
                return true;
            }
        }
        pthread_rwlock_unlock(&seg.seg_rw_lock);
    }

    uint32 sid = LocateSegId(key);
    key_to_seg_[key] = sid;
    Segment& seg = seg_list_[sid];
    pthread_rwlock_wrlock(&seg.seg_rw_lock);
    seg.data_list.emplace_back(key, val);
    seg.total_data_num = static_cast<uint32>(seg.data_list.size());
    pthread_rwlock_unlock(&seg.seg_rw_lock);
    UpdateHotStat(key);

    if (seg.total_data_num > seg_max_cap_)
    {
        SplitSegment(sid);
    }
    return true;
}

bool HSSIIndex::Delete(KeyType key)
{
    std::lock_guard<std::mutex> lk(global_mtx_);
    auto route_it = key_to_seg_.find(key);
    if (route_it == key_to_seg_.end() || route_it->second >= seg_list_.size())
    {
        return false;
    }

    Segment& seg = seg_list_[route_it->second];
    pthread_rwlock_wrlock(&seg.seg_rw_lock);
    auto it = std::find_if(seg.data_list.begin(), seg.data_list.end(), [key](const auto& item) { return item.first == key; });
    if (it == seg.data_list.end())
    {
        pthread_rwlock_unlock(&seg.seg_rw_lock);
        return false;
    }

    seg.data_list.erase(it);
    seg.total_data_num = static_cast<uint32>(seg.data_list.size());
    key_to_seg_.erase(route_it);
    pthread_rwlock_unlock(&seg.seg_rw_lock);
    return true;
}

bool HSSIIndex::PointQuery(KeyType key, ValType& out_val)
{
    std::lock_guard<std::mutex> lk(global_mtx_);
    auto route_it = key_to_seg_.find(key);
    if (route_it == key_to_seg_.end() || route_it->second >= seg_list_.size())
    {
        return false;
    }

    Segment& seg = seg_list_[route_it->second];
    pthread_rwlock_rdlock(&seg.seg_rw_lock);
    for (auto& pair : seg.data_list)
    {
        if (pair.first == key)
        {
            out_val = pair.second;
            pthread_rwlock_unlock(&seg.seg_rw_lock);
            UpdateHotStat(key);
            return true;
        }
    }
    pthread_rwlock_unlock(&seg.seg_rw_lock);
    return false;
}

void HSSIIndex::RangeScan(KeyType l, KeyType r, std::vector<std::pair<KeyType, ValType>>& result)
{
    if (l > r)
    {
        std::swap(l, r);
    }

    std::lock_guard<std::mutex> lk(global_mtx_);
    result.clear();
    for (Segment& seg : seg_list_)
    {
        if (seg.seg_end_key < l || seg.seg_start_key > r)
        {
            continue;
        }

        pthread_rwlock_rdlock(&seg.seg_rw_lock);
        for (auto& p : seg.data_list)
        {
            if (p.first >= l && p.first <= r)
            {
                result.push_back(p);
            }
        }
        pthread_rwlock_unlock(&seg.seg_rw_lock);
    }
}

void HSSIIndex::UpdateHotStat(KeyType key)
{
    auto route_it = key_to_seg_.find(key);
    if (route_it == key_to_seg_.end() || route_it->second >= seg_list_.size())
    {
        return;
    }

    Segment& seg = seg_list_[route_it->second];
    ++seg.access_hot_count;
    seg.is_hot_segment = seg.access_hot_count > hot_threshold_;
}

void HSSIIndex::SplitSegment(uint32 seg_id)
{
    if (seg_id >= seg_list_.size())
    {
        return;
    }

    Segment& old_seg = seg_list_[seg_id];
    if (old_seg.seg_start_key >= old_seg.seg_end_key)
    {
        return;
    }

    KeyType mid_key = old_seg.seg_start_key + (old_seg.seg_end_key - old_seg.seg_start_key) / 2;
    Segment new_seg;
    new_seg.seg_start_key = mid_key + 1;
    new_seg.seg_end_key = old_seg.seg_end_key;
    old_seg.seg_end_key = mid_key;

    uint32 new_seg_id = static_cast<uint32>(seg_list_.size());
    pthread_rwlock_wrlock(&old_seg.seg_rw_lock);
    for (auto it = old_seg.data_list.begin(); it != old_seg.data_list.end();)
    {
        if (it->first > mid_key)
        {
            new_seg.data_list.push_back(*it);
            key_to_seg_[it->first] = new_seg_id;
            it = old_seg.data_list.erase(it);
        }
        else
        {
            ++it;
        }
    }
    old_seg.total_data_num = static_cast<uint32>(old_seg.data_list.size());
    new_seg.total_data_num = static_cast<uint32>(new_seg.data_list.size());
    pthread_rwlock_unlock(&old_seg.seg_rw_lock);

    seg_list_.push_back(std::move(new_seg));
    next_seg_id_ = static_cast<uint32>(seg_list_.size());
    LOG_INFO_STR("Split segment " + std::to_string(seg_id) + ", new seg id=" + std::to_string(new_seg_id));
}

void HSSIIndex::MergeSegment(uint32 seg_a, uint32 seg_b)
{
    if (seg_a >= seg_list_.size() || seg_b >= seg_list_.size() || seg_a == seg_b)
    {
        return;
    }
    if (seg_a > seg_b)
    {
        std::swap(seg_a, seg_b);
    }

    Segment& s1 = seg_list_[seg_a];
    Segment& s2 = seg_list_[seg_b];
    if (s1.seg_end_key + 1 != s2.seg_start_key)
    {
        LOG_WARN_STR("Merge failed, two segments not adjacent");
        return;
    }

    pthread_rwlock_wrlock(&s1.seg_rw_lock);
    pthread_rwlock_wrlock(&s2.seg_rw_lock);
    s1.seg_end_key = s2.seg_end_key;
    s1.data_list.insert(s1.data_list.end(), s2.data_list.begin(), s2.data_list.end());
    s1.total_data_num = static_cast<uint32>(s1.data_list.size());
    s1.access_hot_count += s2.access_hot_count;
    s1.is_hot_segment = s1.is_hot_segment || s2.is_hot_segment;
    for (auto& p : s2.data_list)
    {
        key_to_seg_[p.first] = seg_a;
    }
    pthread_rwlock_unlock(&s2.seg_rw_lock);
    pthread_rwlock_unlock(&s1.seg_rw_lock);

    seg_list_.erase(seg_list_.begin() + seg_b);
    for (auto& item : key_to_seg_)
    {
        if (item.second > seg_b)
        {
            --item.second;
        }
    }
    next_seg_id_ = static_cast<uint32>(seg_list_.size());
    LOG_INFO_STR("Merge seg " + std::to_string(seg_a) + " and " + std::to_string(seg_b));
}
