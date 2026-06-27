#include "config_loader.h"
#include "logger.h"
#include "utils.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace
{
std::string Trim(const std::string& input)
{
    auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) { return std::isspace(ch); });
    auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    if (begin >= end)
    {
        return {};
    }
    return std::string(begin, end);
}

std::string StripInlineComment(const std::string& input)
{
    size_t pos = input.find('#');
    return Trim(input.substr(0, pos));
}

int ParseLogLevel(const std::string& value)
{
    std::string v = value;
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (v == "debug") return LOG_DEBUG;
    if (v == "info") return LOG_INFO;
    if (v == "warn" || v == "warning") return LOG_WARN;
    if (v == "error") return LOG_ERROR;
    return std::stoi(v);
}
}

bool ConfigLoader::LoadConfig(const std::string& file_path, GlobalConfig& cfg)
{
    std::ifstream fin(file_path);
    if (!fin.is_open())
    {
        LOG_ERROR_STR("Open config file failed: " + file_path);
        return false;
    }

    std::string line;
    uint32 line_no = 0;
    try
    {
        while (std::getline(fin, line))
        {
            ++line_no;
            line = StripInlineComment(line);
            if (line.empty())
            {
                continue;
            }

            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos)
            {
                continue;
            }

            std::string key = Trim(line.substr(0, eq_pos));
            std::string val = Trim(line.substr(eq_pos + 1));
            if (key.empty() || val.empty())
            {
                continue;
            }

            if (key == "node_role")
            {
                if (val == "auto") cfg.node_role = ROLE_AUTO;
                else if (val == "meta") cfg.node_role = ROLE_META;
                else if (val == "compute") cfg.node_role = ROLE_COMPUTE;
                else cfg.node_role = ROLE_MEMORY;
            }
            else if (key == "node_id") cfg.node_id = static_cast<uint32>(std::stoul(val));
            else if (key == "cluster_total_nodes") cfg.cluster_total = static_cast<uint32>(std::stoul(val));
            else if (key == "meta_node_ip") cfg.meta_ip = val;
            else if (key == "compute_node_ips") cfg.compute_ips = Utils::SplitString(val, ',');
            else if (key == "memory_node_ips") cfg.memory_ips = Utils::SplitString(val, ',');
            else if (key == "rdma_port") cfg.rdma_port = static_cast<uint16>(std::stoul(val));
            else if (key == "seg_max_capacity") cfg.seg_max_cap = static_cast<uint32>(std::stoul(val));
            else if (key == "seg_min_capacity") cfg.seg_min_cap = static_cast<uint32>(std::stoul(val));
            else if (key == "access_hot_threshold") cfg.hot_threshold = static_cast<uint32>(std::stoul(val));
            else if (key == "load_collect_interval_ms") cfg.collect_interval_ms = static_cast<uint32>(std::stoul(val));
            else if (key == "load_beta_low") cfg.beta_low = std::stof(val);
            else if (key == "load_beta_mid") cfg.beta_mid = std::stof(val);
            else if (key == "load_beta_high") cfg.beta_high = std::stof(val);
            else if (key == "load_beta_max") cfg.beta_max_limit = std::stof(val);
            else if (key == "rdma_single_delay_ns") cfg.rdma_single_delay = static_cast<uint64>(std::stoull(val));
            else if (key == "local_search_delay_ns") cfg.local_search_delay = static_cast<uint64>(std::stoull(val));
            else if (key == "cache_overhead_alpha") cfg.cache_alpha = std::stof(val);
            else if (key == "rpc_base_delay_ns") cfg.rpc_base_delay = static_cast<uint64>(std::stoull(val));
            else if (key == "single_scan_delay_ns") cfg.single_scan_delay = static_cast<uint64>(std::stoull(val));
            else if (key == "lock_sub_block_num") cfg.lock_sub_block = static_cast<uint32>(std::stoul(val));
            else if (key == "cache_base_prob") cfg.cache_base_prob = std::stof(val);
            else if (key == "rl_weight_w1") cfg.rl_w1 = std::stof(val);
            else if (key == "rl_weight_w2") cfg.rl_w2 = std::stof(val);
            else if (key == "rl_weight_w3") cfg.rl_w3 = std::stof(val);
            else if (key == "rl_learning_rate") cfg.rl_learn_rate = std::stof(val);
            else if (key == "rl_discount_factor") cfg.rl_discount = std::stof(val);
            else if (key == "scan_data_coeff") cfg.coeff_scan = std::stof(val);
            else if (key == "leaf_data_coeff") cfg.coeff_leaf = std::stof(val);
            else if (key == "inner_data_coeff") cfg.coeff_inner = std::stof(val);
            else if (key == "l1_numa_cache_size") cfg.l1_cache_size = static_cast<uint32>(std::stoul(val));
            else if (key == "l2_global_cache_size") cfg.l2_cache_size = static_cast<uint32>(std::stoul(val));
            else if (key == "worker_thread_num") cfg.worker_threads = static_cast<uint32>(std::stoul(val));
            else if (key == "log_level") cfg.log_level = ParseLogLevel(val);
            else if (key == "log_save_path") cfg.log_path = val;
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR_STR("Parse config failed at line " + std::to_string(line_no) + ": " + e.what());
        return false;
    }

    LOG_INFO_STR("Load config file success");
    return true;
}
