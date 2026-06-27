#!/bin/bash
mkdir -p ./logs
echo "==================== MemRangeX 单机模式启动 ===================="
nohup ./MemRangeX > ./logs/local_run.log 2>&1 &
echo "单机进程已启动，日志：./logs/local_run.log"
