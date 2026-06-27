#ifndef MEMRANGEX_RD_SERVER_H
#define MEMRANGEX_RD_SERVER_H

#include "../common/common_types.h"
#include "rd_context.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

class RdServer
{
public:
    RdServer(RdContext* ctx, GlobalConfig* cfg);
    ~RdServer();
    void Start();
    void Stop();
    bool SendToNode(const std::string& target_ip, const RdPacket& pkt);

private:
    void RecvLoop();
    void DispatchPacket(const RdPacket& pkt);

    RdContext* rd_ctx_;
    GlobalConfig* global_cfg_;
    std::atomic<bool> running_;
    std::thread recv_thread_;
    std::unordered_map<std::string, rdma_cm_id*> conn_cache_;
    std::mutex conn_mtx_;
};

#endif
