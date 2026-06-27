#include "utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <thread>

TimeNs Utils::GetCurrentNanosecond()
{
    auto tp = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
}

TimeMs Utils::GetCurrentMillisecond()
{
    auto tp = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}

void Utils::SleepMs(TimeMs ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

float Utils::RandomFloat0To1()
{
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(gen);
}

void Utils::BindThreadToNuma(int numa_id)
{
    if (numa_available() < 0 || numa_id < 0)
    {
        return;
    }
    numa_run_on_node(numa_id);
}

std::vector<std::string> Utils::SplitString(const std::string& str, char delim)
{
    std::vector<std::string> res;
    std::string temp;
    std::stringstream ss(str);
    while (std::getline(ss, temp, delim))
    {
        auto begin = std::find_if_not(temp.begin(), temp.end(), [](unsigned char ch) { return std::isspace(ch); });
        auto end = std::find_if_not(temp.rbegin(), temp.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
        if (begin < end)
        {
            res.emplace_back(begin, end);
        }
    }
    return res;
}

uint32 Utils::HashKey(KeyType key)
{
    return static_cast<uint32>(key ^ (key >> 32));
}

Segment::Segment()
    : seg_start_key(0), seg_end_key(0), total_data_num(0), access_hot_count(0), is_hot_segment(false)
{
    pthread_rwlock_init(&seg_rw_lock, nullptr);
}

Segment::~Segment()
{
    pthread_rwlock_destroy(&seg_rw_lock);
}

Segment::Segment(Segment&& other) noexcept
    : seg_start_key(other.seg_start_key),
      seg_end_key(other.seg_end_key),
      total_data_num(other.total_data_num),
      access_hot_count(other.access_hot_count),
      is_hot_segment(other.is_hot_segment),
      data_list(std::move(other.data_list))
{
    pthread_rwlock_init(&seg_rw_lock, nullptr);
}

Segment& Segment::operator=(Segment&& other) noexcept
{
    if (this != &other)
    {
        seg_start_key = other.seg_start_key;
        seg_end_key = other.seg_end_key;
        total_data_num = other.total_data_num;
        access_hot_count = other.access_hot_count;
        is_hot_segment = other.is_hot_segment;
        data_list = std::move(other.data_list);
    }
    return *this;
}

GlobalConfig::GlobalConfig()
    : node_role(ROLE_AUTO),
      node_id(0),
      cluster_total(0),
      rdma_port(0),
      seg_max_cap(0),
      seg_min_cap(0),
      hot_threshold(0),
      collect_interval_ms(10),
      beta_low(1.0f),
      beta_mid(2.0f),
      beta_high(3.0f),
      beta_max_limit(3.0f),
      rdma_single_delay(0),
      local_search_delay(0),
      cache_alpha(1.0f),
      rpc_base_delay(0),
      single_scan_delay(0),
      lock_sub_block(8),
      cache_base_prob(0.5f),
      rl_w1(0.5f),
      rl_w2(0.3f),
      rl_w3(0.2f),
      rl_learn_rate(0.1f),
      rl_discount(0.9f),
      coeff_scan(0.1f),
      coeff_leaf(0.6f),
      coeff_inner(1.0f),
      l1_cache_size(0),
      l2_cache_size(0),
      worker_threads(1),
      log_level(1),
      log_path("./logs/")
{
}
