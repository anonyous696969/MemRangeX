#ifndef MEMRANGEX_COMMON_TYPES_H
#define MEMRANGEX_COMMON_TYPES_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <numa.h>
#include <pthread.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using KeyType = uint64_t;
using ValType = uint64_t;
using TimeNs = uint64_t;
using TimeMs = uint32_t;

enum NodeRole
{
    ROLE_AUTO = 0,
    ROLE_META = 1,
    ROLE_COMPUTE = 2,
    ROLE_MEMORY = 3
};

enum RdmaMsgType
{
    RD_MSG_QUERY = 1,
    RD_MSG_UPDATE = 2,
    RD_MSG_INSERT = 3,
    RD_MSG_SCAN = 4,
    RD_MSG_OFFLOAD = 5,
    RD_MSG_LOAD_SYNC = 6,
    RD_MSG_FAULT = 7
};

enum LoadLevel
{
    LOAD_LEVEL_LOW = 1,
    LOAD_LEVEL_MID = 2,
    LOAD_LEVEL_HIGH = 3
};

struct LoadInfo
{
    float cpu_util;
    uint32 task_queue_len;
    float rdma_bandwidth_util;
    float load_beta;
    LoadLevel level;

    LoadInfo()
        : cpu_util(0.0f), task_queue_len(0), rdma_bandwidth_util(0.0f), load_beta(1.0f), level(LOAD_LEVEL_LOW)
    {
    }
};

struct Segment
{
    KeyType seg_start_key;
    KeyType seg_end_key;
    uint32 total_data_num;
    uint32 access_hot_count;
    bool is_hot_segment;
    std::vector<std::pair<KeyType, ValType>> data_list;
    pthread_rwlock_t seg_rw_lock;

    Segment();
    ~Segment();
    Segment(const Segment&) = delete;
    Segment& operator=(const Segment&) = delete;
    Segment(Segment&& other) noexcept;
    Segment& operator=(Segment&& other) noexcept;
};

struct NodeAddr
{
    std::string ip_addr;
    uint16 rdma_port;
    uint32 node_id;
    int numa_node_id;
};

struct GlobalConfig
{
    NodeRole node_role;
    uint32 node_id;
    uint32 cluster_total;
    std::string meta_ip;
    std::vector<std::string> compute_ips;
    std::vector<std::string> memory_ips;
    uint16 rdma_port;
    uint32 seg_max_cap;
    uint32 seg_min_cap;
    uint32 hot_threshold;
    uint32 collect_interval_ms;
    float beta_low;
    float beta_mid;
    float beta_high;
    float beta_max_limit;
    uint64 rdma_single_delay;
    uint64 local_search_delay;
    float cache_alpha;
    uint64 rpc_base_delay;
    uint64 single_scan_delay;
    uint32 lock_sub_block;
    float cache_base_prob;
    float rl_w1;
    float rl_w2;
    float rl_w3;
    float rl_learn_rate;
    float rl_discount;
    float coeff_scan;
    float coeff_leaf;
    float coeff_inner;
    uint32 l1_cache_size;
    uint32 l2_cache_size;
    uint32 worker_threads;
    int log_level;
    std::string log_path;

    GlobalConfig();
};

#endif
