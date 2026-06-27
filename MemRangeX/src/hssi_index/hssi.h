#ifndef MEMRANGEX_HSSI_H
#define MEMRANGEX_HSSI_H

#include "../common/common_types.h"
#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <vector>

class HSSIIndex
{
public:
    HSSIIndex(uint32 seg_max, uint32 seg_min, uint32 hot_thresh);
    ~HSSIIndex();

    bool Insert(KeyType key, ValType val);
    bool Delete(KeyType key);
    bool PointQuery(KeyType key, ValType& out_val);
    void RangeScan(KeyType l, KeyType r, std::vector<std::pair<KeyType, ValType>>& result);
    void UpdateHotStat(KeyType key);
    void SplitSegment(uint32 seg_id);
    void MergeSegment(uint32 seg_a, uint32 seg_b);
    Segment* GetSegmentById(uint32 seg_id);
    uint32 LocateSegId(KeyType key);
    size_t GetSegmentCount();

private:
    std::mutex global_mtx_;
    std::unordered_map<KeyType, uint32> key_to_seg_;
    std::vector<Segment> seg_list_;
    uint32 next_seg_id_;
    uint32 seg_max_cap_;
    uint32 seg_min_cap_;
    uint32 hot_threshold_;
};

#endif
