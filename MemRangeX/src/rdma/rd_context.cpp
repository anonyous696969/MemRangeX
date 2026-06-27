#include "rd_context.h"
#include "../common/logger.h"
#include <cstring>
#include <netinet/in.h>

RdContext::RdContext()
    : ev_channel_(nullptr), listen_id_(nullptr), protection_domain_(nullptr), completion_q_(nullptr), queue_pair_(nullptr), local_port_(0)
{
}

RdContext::~RdContext()
{
    if (listen_id_ && queue_pair_)
    {
        rdma_destroy_qp(listen_id_);
        queue_pair_ = nullptr;
    }
    if (completion_q_)
    {
        ib_destroy_cq(completion_q_);
        completion_q_ = nullptr;
    }
    if (protection_domain_)
    {
        ib_dealloc_pd(protection_domain_);
        protection_domain_ = nullptr;
    }
    if (listen_id_)
    {
        rdma_destroy_id(listen_id_);
        listen_id_ = nullptr;
    }
    if (ev_channel_)
    {
        rdma_destroy_event_channel(ev_channel_);
        ev_channel_ = nullptr;
    }
}

bool RdContext::Init(uint16 port, NodeRole role)
{
    (void)role;
    local_port_ = port;
    ev_channel_ = rdma_create_event_channel();
    if (!ev_channel_)
    {
        LOG_ERROR_STR("Create RDMA event channel failed");
        return false;
    }

    if (rdma_create_id(ev_channel_, &listen_id_, nullptr, RDMA_PS_TCP) < 0)
    {
        LOG_ERROR_STR("Create RDMA cm id failed");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (rdma_bind_addr(listen_id_, reinterpret_cast<sockaddr*>(&addr)) < 0)
    {
        LOG_ERROR_STR("RDMA bind address failed");
        return false;
    }

    protection_domain_ = ib_alloc_pd(listen_id_->verbs);
    if (!protection_domain_)
    {
        LOG_ERROR_STR("Alloc RDMA PD failed");
        return false;
    }

    completion_q_ = ib_create_cq(listen_id_->verbs, 1024, nullptr, nullptr, 0);
    if (!completion_q_)
    {
        LOG_ERROR_STR("Create RDMA CQ failed");
        return false;
    }

    ibv_qp_init_attr qp_attr{};
    qp_attr.qp_type = IBV_QPT_RC;
    qp_attr.send_cq = completion_q_;
    qp_attr.recv_cq = completion_q_;
    qp_attr.cap.max_send_wr = 1024;
    qp_attr.cap.max_recv_wr = 1024;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_sge = 1;
    if (rdma_create_qp(listen_id_, protection_domain_, &qp_attr) < 0)
    {
        LOG_ERROR_STR("Create RDMA QP failed");
        return false;
    }
    queue_pair_ = listen_id_->qp;

    if (rdma_listen(listen_id_, 5) < 0)
    {
        LOG_ERROR_STR("RDMA listen failed");
        return false;
    }

    LOG_INFO_STR("RDMA context init success, port: " + std::to_string(port));
    return true;
}

bool RdContext::SendPacket(const std::string& dst_ip, const RdPacket& pkt)
{
    (void)dst_ip;
    (void)pkt;
    return true;
}

bool RdContext::RecvPacket(RdPacket& out_pkt)
{
    std::memset(&out_pkt, 0, sizeof(RdPacket));
    return false;
}
