#ifndef MEMRANGEX_FAULT_H
#define MEMRANGEX_FAULT_H

#include "../common/common_types.h"
#include <unordered_set>
#include <mutex>

class FaultHandler
{
public:
    FaultHandler();
    void MarkNodeDown(uint32 node_id);
    bool IsNodeAlive(uint32 node_id);
    void ClearFaultRecord(uint32 node_id);

private:
    std::unordered_set<uint32_t> dead_node_set_;
    std::mutex mtx_;
};

#endif