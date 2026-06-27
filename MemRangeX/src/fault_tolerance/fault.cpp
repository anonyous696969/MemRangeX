#include "fault.h"

FaultHandler::FaultHandler() = default;

void FaultHandler::MarkNodeDown(uint32 node_id)
{
    std::lock_guard<std::mutex> lk(mtx_);
    dead_node_set_.insert(node_id);
}

bool FaultHandler::IsNodeAlive(uint32 node_id)
{
    std::lock_guard<std::mutex> lk(mtx_);
    return dead_node_set_.find(node_id) == dead_node_set_.end();
}

void FaultHandler::ClearFaultRecord(uint32 node_id)
{
    std::lock_guard<std::mutex> lk(mtx_);
    dead_node_set_.erase(node_id);
}