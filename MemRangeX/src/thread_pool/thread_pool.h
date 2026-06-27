#ifndef MEMRANGEX_THREAD_POOL_H
#define MEMRANGEX_THREAD_POOL_H

#include "../common/common_types.h"
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>

class ThreadPool
{
public:
    ThreadPool(uint32 thread_num);
    ~ThreadPool();
    void SubmitTask(std::function<void()> task);
    void Stop();

private:
    void WorkerLoop();
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex queue_mtx_;
    std::condition_variable cond_;
    std::atomic<bool> running_;
};

#endif