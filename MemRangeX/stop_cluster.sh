#!/bin/bash
NODE_LIST=("192.168.10.100" "192.168.10.101" "192.168.10.102" "192.168.10.103" "192.168.10.104")

echo "==================== 停止MemRangeX集群 ===================="
for ip in "${NODE_LIST[@]}"
do
  ssh "$ip" "pkill -f MemRangeX || true"
  echo "节点 ${ip} 进程已终止"
done
