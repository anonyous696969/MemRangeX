#ifndef MEMRANGEX_RD_DEF_H
#define MEMRANGEX_RD_DEF_H

#include "../common/common_types.h"
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>

#define RD_BUFFER_MAX_LEN 8192

struct RdPacket
{
    RdmaMsgType msg_type;
    KeyType key;
    ValType val;
    uint32 segment_id;
    char data_buf[RD_BUFFER_MAX_LEN];
    uint32 data_len;
};

#endif