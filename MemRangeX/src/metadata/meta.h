#ifndef MEMRANGEX_META_H
#define MEMRANGEX_META_H

#include "../common/common_types.h"
#include <unordered_map>
#include <mutex>

class MetaNode
{
public:
    MetaNode(GlobalConfig* cfg);
    
    void UpdateNodeLoad(uint32 node_id, const LoadInfo& info);
    
    std::unordered_map<uint32_t, LoadInfo> GetGlobalLoadView();
    
    void AddSegRoute(uint32 seg_id, uint32 mem_node_id);
    uint32 GetMemNodeBySeg(uint32 seg_id);

private:
    GlobalConfig* cfg_;
    std::unordered_map<uint32_t, LoadInfo> node_load_map_;
    std::unordered_map<uint32_t, uint32_t> seg_route_map_;
    std::mutex load_mtx_;
    std::mutex route_mtx_;
};

#endif