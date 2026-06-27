#!/bin/bash
NODE_LIST=("192.168.10.100" "192.168.10.101" "192.168.10.102" "192.168.10.103" "192.168.10.104")
PROJECT_DIR=$(pwd)
mkdir -p ./logs

echo "==================== RDMA集群启动 ===================="
for ip in "${NODE_LIST[@]}"
do
  ssh "$ip" "cd '$PROJECT_DIR' && mkdir -p ./logs && nohup ./MemRangeX > ./logs/node_${ip}.log 2>&1 &"
  echo "节点 ${ip} 启动完成"
done
echo "集群全部启动，查看日志：./logs/"
