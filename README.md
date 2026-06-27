MemRangeX

项目简介

MemRangeX 是面向CXL/RDMA 内存分离架构的高性能分布式范围索引系统，彻底摒弃传统 B+/ 基数树树形索引架构，自研分层有序段索引 HSSI，配套四大核心创新模块，支持真实 InfiniBand RDMA 集群部署、多 NUMA 服务器硬件适配、YCSB 标准负载压测。

核心创新

1.HSSI 分层有序段索引：连续有序键段存储，原生适配 RDMA 批量传输，解决树形索引范围查询碎片化请求问题；内置键段自动分裂、合并、灰度迁移机制。
2.负载感知混合卸载引擎：完整实现论文三套代价计算公式，结合全局节点负载系数 β 动态决策是否远程卸载查询 / 扫描任务，内置过载限流逻辑。
3.NUMA 多层混合并发控制框架：M=8 细粒度子块读写锁 + HTM 硬件事务降级机制，实现锁冲突概率数学模型计算，大幅降低跨 NUMA 访问延迟。
4.三级 NUMA 缓存 + Q-Learning 在线强化学习自适应缓存：完整实现缓存准入概率公式、奖励函数，无人工静态参数，动态适配读写 / 扫描混合负载。

配套支撑模块

1.全局周期性负载采集模块（10ms 采集周期）
2.Raft 元数据路由集群（键段映射、全局负载视图同步）
3.全链路故障容错模块（节点宕机标记、流量重分配）
4.标准化 YCSB 五类负载驱动（纯查询 / 读密集 / 写密集 / 插入 / 扫描）
5.高性能 RDMA 原生通信层（libibverbs/librdmacm，非模拟实现）
6.可配置线程池、分级日志系统、全局参数配置解析器

一、环境硬性依赖

1.1 硬件要求
多 NUMA x86 服务器（双路 / 四路 Intel Xeon）
Mellanox ConnectX-5/6 InfiniBand RDMA 网卡（100Gbps）
大容量 DRAM（单节点 ≥256GB）

1.2 操作系统 & 内核
推荐：Ubuntu 22.04 LTS / CentOS Stream 9
Linux Kernel ≥5.15（推荐 6.3+，原生支持 CXL/NUMA 优化）

1.3 软件编译依赖
# 更新源
sudo apt update && sudo apt upgrade -y

# RDMA/InfiniBand 套件
sudo apt install -y ibutils ibverbs-utils rdmacm-utils libibverbs-dev librdmacm-dev

# NUMA 开发库与工具
sudo apt install -y libnuma-dev numactl

# C++17 编译工具链
sudo apt install -y gcc-13 g++-13 cmake make

# 网络调试工具
sudo apt install -y net-tools iputils-ping openssh-server

1.4 RDMA 内核模块加载（所有集群节点执行）

# 永久加载内核驱动
echo "ib_ipoib" | sudo tee -a /etc/modules-load.d/rdma.conf
echo "mlx5_core" | sudo tee -a /etc/modules-load.d/rdma.conf
echo "rdma_cm" | sudo tee -a /etc/modules-load.d/rdma.conf

# 临时加载生效
sudo modprobe ib_ipoib mlx5_core rdma_cm

# 校验网卡是否识别（正常输出网卡信息即成功）
ibstat
numactl --hardware


二、工程目录结构

MemRangeX/
├── README.md                # 项目说明文档（本文档）
├── config.ini               # 全局运行参数配置文件
├── CMakeLists.txt          # 完整编译构建脚本
├── run_local.sh            # 单机启动脚本
├── run_cluster_4node.sh    # 4节点RDMA集群一键启动
├── stop_cluster.sh         # 集群进程批量停止脚本
├── clear_log.sh            # 日志清理脚本
├── logs/                   # 运行日志自动输出目录
└── src/
    ├── common/              # 公共基础组件
    │   ├── common_types.h   # 全局数据结构、枚举定义
    │   ├── utils.h/cpp      # 时间、NUMA绑定、哈希、字符串工具
    │   ├── logger.h/cpp     # 分级日志系统（DEBUG/INFO/WARN/ERROR）
    │   ├── config_loader.h/cpp # ini配置解析器
    ├── rdma/                # 原生RDMA通信层
    │   ├── rd_def.h         # RDMA数据包定义
    │   ├── rd_context.h/cpp # RDMA底层QP/CQ/PD上下文管理
    │   ├── rd_server.h/cpp  # RDMA后台监听服务、消息分发
    ├── numa_concurrency/    # NUMA多层混合锁模块
    │   ├── numa_lock.h/cpp  # 细粒度子块锁、HTM事务、冲突概率计算
    ├── hssi_index/          # HSSI分层有序段索引核心
    │   ├── hssi.h/cpp       # 增删改查、范围扫描、段分裂合并
    ├── load_collector/      # 全局负载采集监控
    │   ├── load_monitor.h/cpp # CPU/队列/带宽采集、负载系数β计算
    ├── offload_engine/      # 负载感知卸载引擎（论文代价模型）
    │   ├── offload.h/cpp    # 点查询/扫描卸载代价完整计算公式实现
    ├── cache_system/        # 三级缓存+强化学习自适应决策
    │   ├── l1_numa_cache.h/cpp  # NUMA私有L1缓存
    │   ├── l2_global_cache.h/cpp # 整机共享L2缓存
    │   ├── rl_agent.h/cpp        # Q-Learning缓存准入、奖励函数
    ├── segment_manager/     # 键段自动管理后台线程
    │   ├── seg_mgr.h/cpp    # 周期检测分裂/合并阈值
    ├── fault_tolerance/     # 集群故障容错
    │   ├── fault.h/cpp      # 节点宕机标记、存活判断
    ├── metadata/            # Raft元数据节点（路由+负载视图）
    │   ├── meta.h/cpp       # 键段路由映射、全局负载同步
    ├── thread_pool/         # 通用高性能线程池
    │   ├── thread_pool.h/cpp
    ├── ycsb_driver/         # YCSB标准负载压测驱动
    │   ├── ycsb.h/cpp       # 五类业务负载完整实现
    └── main.cpp             # 程序唯一入口，全模块初始化、常驻服务


三、配置文件说明 config.ini
所有集群节点必须保持完全一致，核心参数分为六大模块：集群网络、HSSI 索引阈值、负载采集、卸载代价模型、强化学习缓存、线程日志。


# ===================== 集群网络配置 =====================
node_role = auto                # 节点角色：auto/meta/compute/memory
node_id = 0                     # 当前节点编号
cluster_total_nodes = 4         # 集群总节点数量
meta_node_ip = 192.168.10.100   # 元数据节点IP
compute_node_ips = 192.168.10.101,192.168.10.102
memory_node_ips = 192.168.10.103,192.168.10.104
rdma_port = 12345               # RDMA通信端口

# ===================== HSSI分段索引阈值（论文公式参数） =====================
seg_max_capacity = 1024000      # 单段最大存储数据量 Cap_max
seg_min_capacity = 1024         # 单段最小存储数据量 Cap_min
access_hot_threshold = 1000     # 段热度判定阈值 Hot_max

# ===================== 负载采集配置 =====================
load_collect_interval_ms = 10   # 负载采集周期10ms（论文标准）
load_beta_low = 1.0
load_beta_mid = 2.0
load_beta_high = 3.0
load_beta_max = 3.0             # 负载上限β_max，超过则关闭任务卸载

# ===================== RDMA卸载代价模型参数 =====================
rdma_single_delay_ns = 30       # t_r 单次RDMA远程访问延迟
local_search_delay_ns = 8       # t_l 本地内存检索延迟
cache_overhead_alpha = 1.2      # α 缓存访问开销系数
rpc_base_delay_ns = 60          # t_rpc RPC远程调用基础延迟
single_scan_delay_ns = 1        # t_s 单条数据遍历延迟

# ===================== 锁冲突模型参数 =====================
lock_sub_block_num = 8          # M=8 子块数量（论文固定参数）

# ===================== 强化学习缓存参数 =====================
cache_base_prob = 0.5           # P_base 缓存准入基准概率
rl_weight_w1 = 0.5              # 奖励函数权重ω1
rl_weight_w2 = 0.3              # 奖励函数权重ω2
rl_weight_w3 = 0.2              # 奖励函数权重ω3
rl_learning_rate = 0.1          # Q学习率
rl_discount_factor = 0.9        # 折扣因子
scan_data_coeff = 0.1           # 扫描数据准入衰减系数γ(Type)
leaf_data_coeff = 0.6           # 叶子节点数据系数
inner_data_coeff = 1.0          # 索引内部节点系数

# ===================== 缓存容量配置 =====================
l1_numa_cache_size = 65536      # 单NUMA域L1缓存容量
l2_global_cache_size = 262144   # 整机共享L2缓存容量

# ===================== 线程与日志配置 =====================
worker_thread_num = 36          # 业务工作线程数，匹配CPU核心
log_level = info                # 日志等级：debug/info/warn/error
log_save_path = ./logs/         # 日志输出路径



四、编译构建流程

4.1 创建编译目录
mkdir build && cd build


4.2 CMake 生成构建文件

cmake ..

4.3 全量编译（使用所有 CPU 核心加速）

make -j$(nproc)

五、运行部署指南

5.1 单机本地测试（单节点验证功能）

# 进入编译目录
cd build
# 赋予脚本执行权限
chmod +x run_local.sh
# 启动单机服务
./run_local.sh
# 查看实时日志
tail -f ./logs/memrangex_runtime.log



5.2 4 节点 RDMA 分布式集群启动

1.将完整工程同步至全部 4 台服务器，保持路径一致

2.修改所有节点config.ini内 IP 地址与实际集群匹配

3.所有节点赋予脚本执行权限
chmod +x run_cluster_4node.sh stop_cluster.sh clear_log.sh

4.在任意一台节点执行集群启动脚本
./run_cluster_4node.sh

5.查看各节点独立日志：./logs/node_${节点IP}.log

5.3 集群停止
./stop_cluster.sh

5.4 清空历史日志
./clear_log.sh


编译运行常见问题排查

1.RDMA 初始化失败
检查网卡驱动是否加载：ibstat
关闭防火墙，开放 12345 端口
集群节点 IP 互通，InfiniBand 网卡同网段

2. libnuma 链接报错
sudo apt install libnuma-dev

3. GCC 版本过低
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100







