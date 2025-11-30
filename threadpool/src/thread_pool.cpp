#include <thread>
#include <chrono>
#include <cstdio>
#include <iostream>

#include "thread_pool.hpp"

ThreadPool::ThreadPool(int num_threads)
    : num_threads(num_threads)
{
    workers = new Worker[num_threads];
    for (int i = 0; i < num_threads; ++i)
    {
        workers[i].id = i;
    }

    threads = new std::thread[num_threads];
    for (int i = 0; i < num_threads; ++i)
    {
        threads[i] = std::thread(worker_routine, this, &workers[i]);
    }
}

ThreadPool::~ThreadPool()
{
    is_stopped.store(true, std::memory_order_release);
    for (int i = 0; i < num_threads; ++i)
    {
        if (threads[i].joinable())
        {
            threads[i].join();
        }
    }
    delete[] threads;
    delete[] workers;
}

void ThreadPool::submit(Task* task_ptr)
{
    int worker_id1 = rand_gen.get(0, num_threads - 1);
    int worker_id2 = rand_gen.get(0, num_threads - 1);
    if (workers[worker_id1].get_deque_size() <= workers[worker_id2].get_deque_size())
    {
        workers[worker_id1].submit(task_ptr);
    }
    else
    {
        workers[worker_id2].submit(task_ptr);
    }
}

void ThreadPool::submit(task_func_t func, void* arg)
{
    Task* t = new Task;
    t->func = func;
    t->arg = arg;
    submit(t);
}

void ThreadPool::wait_completion()
{
    for (int i = 0; i < num_threads; ++i)
    {
        workers[i].wait_completion();
    }
}

void* ThreadPool::worker_routine(ThreadPool* pool, Worker* worker)
{
    while (!pool->is_stopped.load(std::memory_order_acquire))
    {
        Task *task = worker->deque.pop_bottom();
        if (task)
        {
            task->func(task->arg);
            delete task;
        }
        else
        {
            int worker_id1 = pool->rand_gen.get(0, pool->num_threads - 1);
            int worker_id2 = pool->rand_gen.get(0, pool->num_threads - 1);
            if (pool->workers[worker_id1].get_deque_size() >= pool->workers[worker_id2].get_deque_size())
            {
                task = pool->workers[worker_id1].steal_task();
                if (task)
                {
                    std::cout << "[worker " << worker->id << "] stole task from " << worker_id1 << "\n";
                }
            }
            else
            {
                task = pool->workers[worker_id2].steal_task();
                if (task)
                {
                    std::cout << "[worker " << worker->id << "] stole task from " << worker_id2 << "\n";
                }
            }

            if (task)
            {
                task->func(task->arg);
                delete task;
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100 + pool->rand_gen.get(worker->id * 10, 100 + worker->id * 10)));
            }
        }
    }
    return nullptr;
}