#ifndef MEMRANGEX_RD_CONTEXT_H
#define MEMRANGEX_RD_CONTEXT_H

#include "rd_def.h"
#include <unordered_map>
#include <string>

class RdContext
{
public:
    RdContext();
    ~RdContext();
    bool Init(uint16 port, NodeRole role);
    bool SendPacket(const std::string& dst_ip, const RdPacket& pkt);
    bool RecvPacket(RdPacket& out_pkt);

private:
    rdma_event_channel* ev_channel_;
    rdma_cm_id* listen_id_;
    ib_pd* protection_domain_;
    ib_cq* completion_q_;
    ib_qp* queue_pair_;
    uint16 local_port_;
};

#endif