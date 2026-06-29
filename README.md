# MemRangeX

## Project Overview

MemRangeX is a high-performance distributed range indexing system designed for CXL/RDMA-based memory disaggregation architectures. Instead of following conventional tree-based indexing structures such as B+ trees or radix trees, MemRangeX adopts a self-designed Hierarchical Sorted-Segment Index (HSSI). The system integrates four core modules and supports deployment on real InfiniBand RDMA clusters, hardware-aware execution on multi-NUMA servers, and YCSB-style workload evaluation.

## Core Contributions

1. **HSSI: Hierarchical Sorted-Segment Index**
   HSSI stores data in contiguous and ordered key segments, which naturally fits RDMA batch transfer and reduces fragmented remote accesses in range queries. It also supports automatic segment splitting, merging, and gradual migration.

2. **Load-Aware Hybrid Offloading Engine**
   MemRangeX implements cost models for query and scan offloading. By combining these models with a global node load factor β, the system dynamically decides whether to offload query or scan tasks to remote memory-side nodes. It also includes overload-aware throttling logic.

3. **NUMA-Aware Multi-Level Hybrid Concurrency Control**
   MemRangeX introduces a NUMA-aware concurrency control framework with M=8 fine-grained sub-block read-write locks and a hardware transactional memory fallback mechanism. It also includes a mathematical model for estimating lock conflict probability, reducing cross-NUMA access overhead.

4. **Three-Level NUMA Cache with Q-Learning-Based Adaptive Admission**
   MemRangeX implements cache admission probability modeling and a Q-learning-based reward function. The cache admission policy does not rely on manually fixed static parameters and can dynamically adapt to mixed read, write, and scan workloads.

## Supporting Modules

1. Periodic global load collection module with a 10 ms collection interval.
2. Raft-style metadata routing cluster for key-segment mapping and global load-view synchronization.
3. End-to-end fault tolerance module for node failure marking and traffic redistribution.
4. YCSB-style workload driver for query, read-intensive, write-intensive, insert-intensive, and scan-intensive workloads.
5. Native high-performance RDMA communication layer based on `libibverbs` and `librdmacm`.
6. Configurable thread pool, hierarchical logging system, and global configuration parser.

---

# 1. System Requirements

## 1.1 Hardware Requirements

* Multi-NUMA x86 servers, such as dual-socket or quad-socket Intel Xeon servers.
* Mellanox ConnectX-5 or ConnectX-6 InfiniBand RDMA NICs, preferably 100 Gbps.
* Large DRAM capacity, with at least 256 GB per node recommended.

## 1.2 Operating System and Kernel

Recommended operating systems:

* Ubuntu 22.04 LTS
* CentOS Stream 9

Recommended Linux kernel:

* Linux Kernel 5.15 or later
* Linux Kernel 6.3 or later is preferred for improved CXL/NUMA support.

## 1.3 Software Dependencies

Update the package source:

```bash
sudo apt update && sudo apt upgrade -y
```

Install RDMA and InfiniBand packages:

```bash
sudo apt install -y ibutils ibverbs-utils rdmacm-utils libibverbs-dev librdmacm-dev
```

Install NUMA libraries and tools:

```bash
sudo apt install -y libnuma-dev numactl
```

Install the C++17 build toolchain:

```bash
sudo apt install -y gcc-13 g++-13 cmake make
```

Install network debugging tools:

```bash
sudo apt install -y net-tools iputils-ping openssh-server
```

## 1.4 RDMA Kernel Module Loading

Run the following commands on all cluster nodes.

Enable persistent RDMA kernel module loading:

```bash
echo "ib_ipoib" | sudo tee -a /etc/modules-load.d/rdma.conf
echo "mlx5_core" | sudo tee -a /etc/modules-load.d/rdma.conf
echo "rdma_cm" | sudo tee -a /etc/modules-load.d/rdma.conf
```

Load the modules immediately:

```bash
sudo modprobe ib_ipoib mlx5_core rdma_cm
```

Check whether the RDMA NIC and NUMA topology are correctly detected:

```bash
ibstat
numactl --hardware
```

---

# 2. Project Directory Structure

```text
MemRangeX/
├── README.md
├── config.ini
├── CMakeLists.txt
├── run_local.sh
├── run_cluster_4node.sh
├── stop_cluster.sh
├── clear_log.sh
├── logs/
└── src/
    ├── common/
    │   ├── common_types.h
    │   ├── utils.h/cpp
    │   ├── logger.h/cpp
    │   └── config_loader.h/cpp
    ├── rdma/
    │   ├── rd_def.h
    │   ├── rd_context.h/cpp
    │   └── rd_server.h/cpp
    ├── numa_concurrency/
    │   └── numa_lock.h/cpp
    ├── hssi_index/
    │   └── hssi.h/cpp
    ├── load_collector/
    │   └── load_monitor.h/cpp
    ├── offload_engine/
    │   └── offload.h/cpp
    ├── cache_system/
    │   ├── l1_numa_cache.h/cpp
    │   ├── l2_global_cache.h/cpp
    │   └── rl_agent.h/cpp
    ├── segment_manager/
    │   └── seg_mgr.h/cpp
    ├── fault_tolerance/
    │   └── fault.h/cpp
    ├── metadata/
    │   └── meta.h/cpp
    ├── thread_pool/
    │   └── thread_pool.h/cpp
    ├── ycsb_driver/
    │   └── ycsb.h/cpp
    └── main.cpp
```

Directory descriptions:

* `common/`: Common data structures, utility functions, logger, and configuration parser.
* `rdma/`: Native RDMA communication layer, including packet definitions, QP/CQ/PD context management, server listener, and message dispatching.
* `numa_concurrency/`: NUMA-aware hybrid locking module with fine-grained sub-block locks and HTM fallback.
* `hssi_index/`: Core implementation of the HSSI range index, including insertion, deletion, update, lookup, range scan, segment splitting, and segment merging.
* `load_collector/`: Global load monitoring module, including CPU, queue, bandwidth, and load-factor calculation.
* `offload_engine/`: Load-aware offloading engine implementing the cost model for point queries and range scans.
* `cache_system/`: Three-level cache system with Q-learning-based adaptive cache admission.
* `segment_manager/`: Background segment management thread for periodic split and merge checking.
* `fault_tolerance/`: Cluster fault-tolerance module for node failure detection and traffic redistribution.
* `metadata/`: Metadata routing and global load-view synchronization.
* `thread_pool/`: General-purpose high-performance thread pool.
* `ycsb_driver/`: YCSB-style workload driver.
* `main.cpp`: Main program entry point for module initialization and runtime service execution.

---

# 3. Configuration File

The configuration file is `config.ini`. All cluster nodes should use consistent configuration values unless node-specific parameters are explicitly required.

The configuration parameters are divided into six categories:

1. Cluster and network configuration.
2. HSSI segment-index thresholds.
3. Load collection parameters.
4. RDMA offloading cost model parameters.
5. Reinforcement-learning cache parameters.
6. Thread and logging parameters.

Example configuration:

```ini
node_role = auto
node_id = 0
cluster_total_nodes = 4
meta_node_ip = 192.168.10.100
compute_node_ips = 192.168.10.101,192.168.10.102
memory_node_ips = 192.168.10.103,192.168.10.104
rdma_port = 12345

seg_max_capacity = 1024000
seg_min_capacity = 1024
access_hot_threshold = 1000

load_collect_interval_ms = 10
load_beta_low = 1.0
load_beta_mid = 2.0
load_beta_high = 3.0
load_beta_max = 3.0

rdma_single_delay_ns = 30
local_search_delay_ns = 8
cache_overhead_alpha = 1.2
rpc_base_delay_ns = 60
single_scan_delay_ns = 1

lock_sub_block_num = 8

cache_base_prob = 0.5
rl_weight_w1 = 0.5
rl_weight_w2 = 0.3
rl_weight_w3 = 0.2
rl_learning_rate = 0.1
rl_discount_factor = 0.9
scan_data_coeff = 0.1
leaf_data_coeff = 0.6
inner_data_coeff = 1.0

l1_numa_cache_size = 65536
l2_global_cache_size = 262144

worker_thread_num = 36
log_level = info
log_save_path = ./logs/
```

---

# 4. Build Instructions

## 4.1 Create a Build Directory

```bash
mkdir build && cd build
```

## 4.2 Generate Build Files with CMake

```bash
cmake ..
```

## 4.3 Compile the Project

```bash
make -j$(nproc)
```

---

# 5. Deployment and Execution

## 5.1 Single-Node Local Test

Enter the build directory:

```bash
cd build
```

Grant execution permission to the local run script:

```bash
chmod +x run_local.sh
```

Start the local service:

```bash
./run_local.sh
```

View runtime logs:

```bash
tail -f ./logs/memrangex_runtime.log
```

## 5.2 Four-Node RDMA Cluster Deployment

1. Synchronize the complete project directory to all four servers and keep the same path on each node.

2. Modify the IP addresses in `config.ini` on all nodes according to the actual cluster environment.

3. Grant execution permissions to the cluster scripts:

```bash
chmod +x run_cluster_4node.sh stop_cluster.sh clear_log.sh
```

4. Start the cluster from any node:

```bash
./run_cluster_4node.sh
```

5. Check the log file of each node:

```bash
./logs/node_${node_ip}.log
```

## 5.3 Stop the Cluster

```bash
./stop_cluster.sh
```

## 5.4 Clear Historical Logs

```bash
./clear_log.sh
```

---

# 6. Troubleshooting

## 6.1 RDMA Initialization Failure

Check whether the RDMA NIC driver is correctly loaded:

```bash
ibstat
```

Also check the following items:

* Make sure the firewall is disabled or port `12345` is open.
* Make sure all cluster nodes can reach each other through the InfiniBand network.
* Make sure the RDMA NICs are in the same network segment.
* Make sure `libibverbs` and `librdmacm` are correctly installed.

## 6.2 `libnuma` Linking Error

Install the NUMA development library:

```bash
sudo apt install -y libnuma-dev
```

## 6.3 GCC Version Is Too Low

Install and configure GCC 13:

```bash
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
```

Then verify the compiler version:

```bash
gcc --version
g++ --version
```

---

# 7. Notes

* The system is intended for research and experimental evaluation of range indexing over disaggregated memory.
* RDMA-related functionality requires real RDMA-capable hardware and a properly configured InfiniBand environment.
* Configuration values should be adjusted according to the cluster size, NUMA topology, RDMA network, and workload settings.
* For artifact evaluation or paper reproduction, please keep the hardware, kernel, compiler, and configuration parameters consistent across all nodes.
