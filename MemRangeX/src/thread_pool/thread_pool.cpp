#include "thread_pool.h"
#include <utility>

ThreadPool::ThreadPool(uint32 thread_num)
    : running_(true)
{
    if (thread_num == 0)
    {
        thread_num = 1;
    }
    for (uint32 i = 0; i < thread_num; ++i)
    {
        workers_.emplace_back(&ThreadPool::WorkerLoop, this);
    }
}

ThreadPool::~ThreadPool()
{
    Stop();
}

void ThreadPool::SubmitTask(std::function<void()> task)
{
    if (!running_.load())
    {
        return;
    }
    {
        std::lock_guard<std::mutex> lk(queue_mtx_);
        task_queue_.push(std::move(task));
    }
    cond_.notify_one();
}

void ThreadPool::Stop()
{
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false) && workers_.empty())
    {
        return;
    }
    cond_.notify_all();
    for (auto& t : workers_)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    workers_.clear();
}

void ThreadPool::WorkerLoop()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lk(queue_mtx_);
            cond_.wait(lk, [this]() { return !task_queue_.empty() || !running_.load(); });
            if (!running_.load() && task_queue_.empty())
            {
                break;
            }
            task = std::move(task_queue_.front());
            task_queue_.pop();
        }
        task();
    }
}
