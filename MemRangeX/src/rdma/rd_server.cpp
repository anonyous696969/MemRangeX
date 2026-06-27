#include "rd_server.h"
#include "../common/logger.h"
#include "../common/utils.h"
#include "../hssi_index/hssi.h"
#include "../offload_engine/offload.h"
#include <cstring>

RdServer::RdServer(RdContext* ctx, GlobalConfig* cfg)
    : rd_ctx_(ctx), global_cfg_(cfg), running_(false)
{
}

RdServer::~RdServer()
{
    Stop();
    std::lock_guard<std::mutex> lk(conn_mtx_);
    for (auto& pair : conn_cache_)
    {
        if (pair.second != nullptr)
        {
            rdma_destroy_id(pair.second);
        }
    }
    conn_cache_.clear();
}

void RdServer::Start()
{
    if (running_.load())
    {
        LOG_WARN_STR("RDMA server already running");
        return;
    }
    running_ = true;
    recv_thread_ = std::thread(&RdServer::RecvLoop, this);
    LOG_INFO_STR("RDMA server recv loop started");
}

void RdServer::Stop()
{
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false) && !recv_thread_.joinable())
    {
        return;
    }
    if (recv_thread_.joinable())
    {
        recv_thread_.join();
    }
    LOG_INFO_STR("RDMA server stopped");
}

bool RdServer::SendToNode(const std::string& target_ip, const RdPacket& pkt)
{
    return rd_ctx_->SendPacket(target_ip, pkt);
}

void RdServer::RecvLoop()
{
    Utils::BindThreadToNuma(0);
    RdPacket recv_pkt{};
    while (running_.load())
    {
        if (!rd_ctx_->RecvPacket(recv_pkt))
        {
            Utils::SleepMs(1);
            continue;
        }
        DispatchPacket(recv_pkt);
        std::memset(&recv_pkt, 0, sizeof(RdPacket));
    }
}

void RdServer::DispatchPacket(const RdPacket& pkt)
{
    (void)global_cfg_;
    switch (pkt.msg_type)
    {
        case RD_MSG_QUERY:
            LOG_DEBUG_STR("Recv RDMA point query packet, key=" + std::to_string(pkt.key));
            break;
        case RD_MSG_UPDATE:
            LOG_DEBUG_STR("Recv RDMA update packet, key=" + std::to_string(pkt.key));
            break;
        case RD_MSG_INSERT:
            LOG_DEBUG_STR("Recv RDMA insert packet, key=" + std::to_string(pkt.key));
            break;
        case RD_MSG_SCAN:
            LOG_DEBUG_STR("Recv RDMA range scan packet");
            break;
        case RD_MSG_OFFLOAD:
            LOG_DEBUG_STR("Recv RDMA offload task packet");
            break;
        case RD_MSG_LOAD_SYNC:
            LOG_DEBUG_STR("Recv global load view sync packet");
            break;
        case RD_MSG_FAULT:
            LOG_WARN_STR("Recv node fault notification packet");
            break;
        default:
            LOG_WARN_STR("Unknown RDMA msg type: " + std::to_string(static_cast<uint32>(pkt.msg_type)));
            break;
    }
}
