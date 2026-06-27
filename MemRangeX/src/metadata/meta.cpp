#include "meta.h"

MetaNode::MetaNode(GlobalConfig* cfg) : cfg_(cfg)
{
}

void MetaNode::UpdateNodeLoad(uint32 node_id, const LoadInfo& info)
{
    std::lock_guard<std::mutex> lk(load_mtx_);
    node_load_map_[node_id] = info;
}

std::unordered_map<uint32_t, LoadInfo> MetaNode::GetGlobalLoadView()
{
    std::lock_guard<std::mutex> lk(load_mtx_);
    return node_load_map_;
}

void MetaNode::AddSegRoute(uint32 seg_id, uint32 mem_node_id)
{
    std::lock_guard<std::mutex> lk(route_mtx_);
    seg_route_map_[seg_id] = mem_node_id;
}

uint32 MetaNode::GetMemNodeBySeg(uint32 seg_id)
{
    std::lock_guard<std::mutex> lk(route_mtx_);
    if (!seg_route_map_.count(seg_id)) return 0;
    return seg_route_map_[seg_id];
}