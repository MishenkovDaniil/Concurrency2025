#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <cstddef>
#include <thread>
#include <stddef.h>
#include <atomic>
#include "chase_lev_deque.hpp"
#include "simple_rand_gen.hpp"

typedef void* (*task_func_t)(void*);
struct Task{
    task_func_t func;
    void* arg;
};

class Worker
{
public:
    int id;
    ChaseLevDeque<Task> deque;

public:
    Worker() : deque(1024) {}
    ~Worker() {}
    void submit(Task* task_ptr)
    {
        deque.push_bottom(task_ptr);
    }
    void wait_completion()
    {
        while (deque.get_size())
        {
            Task *task = deque.pop_bottom();
            if (task)
            {
                task->func(task->arg);
                delete task;
            }
        }
    }
    size_t get_deque_size()
    {
        return deque.get_size();
    }
    Task *steal_task()
    {
        return deque.steal_top();
    }
};

class ThreadPool
{
public:
    int num_threads;
    class Worker *workers;
private:
    SimpleRandGen rand_gen;
    std::thread* threads;
    std::atomic<bool> is_stopped{false};

public:
    ThreadPool(int num_threads);
    ~ThreadPool();
    void submit(Task* task_ptr);
    void submit(task_func_t func, void* arg);
    void wait_completion();
    void stop();
private:
    static void* worker_routine(ThreadPool* pool, Worker* worker);
};

#endif /* THREAD_POOL_H */
