#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <thread>

#include "hp.hpp"

// Exponential backoff with random jitter
inline void exponential_backoff(int attempt)
{
    if (attempt == 0)
        return;

    int max_delay_ns = std::min(1 << attempt, 100) * 10;

    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(0, max_delay_ns);
    int delay_ns = dis(gen);

    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::high_resolution_clock::now() - start < std::chrono::nanoseconds(delay_ns))
    {
        asm volatile("pause");
    }
}

struct data_to_reclaim_base
{
    void *m_data;
    data_to_reclaim_base *m_next;
    std::function<void(void *)> m_deleter;

    data_to_reclaim_base(void *data_, std::function<void(void *)> deleter_)
        : m_data(data_), m_next(nullptr), m_deleter(deleter_)
    {
    }

    virtual ~data_to_reclaim_base() = default;
};

template <typename T> struct data_to_reclaim : public data_to_reclaim_base
{
    data_to_reclaim(T *data_)
        : data_to_reclaim_base(static_cast<void *>(data_), [](void *p) { delete static_cast<T *>(p); })
    {
    }
};

template <typename T> void do_delete(void *p);
void add_to_reclaim_list(data_to_reclaim_base *node);
void delete_nodes_with_no_hazards();

extern std::atomic<data_to_reclaim_base *> nodes_to_reclaim;
extern std::atomic<size_t> nodes_to_reclaim_count;
extern std::atomic<bool> reclamation_in_progress;

template <typename T> void reclaim_later(T *data)
{
    nodes_to_reclaim_count.fetch_add(1, std::memory_order_release);
    add_to_reclaim_list(new data_to_reclaim<T>(data));
}

template <typename T> class LFStack
{
  private:
    struct Node
    {
        std::shared_ptr<T> m_data;
        Node *m_next;
        Node(T const &data_) : m_data(std::make_shared<T>(data_))
        {
        }
    };
    std::atomic<Node *> m_head;

  public:
    LFStack() : m_head(nullptr)
    {
    }
    ~LFStack()
    {
        while (auto res = pop())
        {
        }

        delete_nodes_with_no_hazards();
    }
    void push(T const &data)
    {
        Node *new_node = new Node(data);
        new_node->m_next = m_head.load(std::memory_order_relaxed);
        int attempts = 0;
        while (!m_head.compare_exchange_weak(new_node->m_next, new_node, std::memory_order_release,
                                             std::memory_order_relaxed))
        {
            exponential_backoff(attempts);
            attempts = (attempts < 10) ? attempts + 1 : 10; // up to 10
        }
    }

    std::shared_ptr<T> pop(void)
    {
        std::atomic<void *> &hp = get_hazard_pointer_for_current_thread();
        Node *old_head = m_head.load(std::memory_order_acquire);
        Node *temp = nullptr;
        int attempts = 0;

        while (true)
        {
            do
            {
                temp = old_head;
                hp.store(old_head);
                old_head = m_head.load(std::memory_order_relaxed);
            } while (old_head != temp);

            if (!old_head)
                break;

            if (m_head.compare_exchange_strong(old_head, old_head->m_next, std::memory_order_release,
                                               std::memory_order_relaxed))
                break;

            // apply backoff only if already failed to change head
            exponential_backoff(attempts);
            attempts = (attempts < 10) ? attempts + 1 : 10;
            old_head = m_head.load(std::memory_order_acquire);
        }

        hp.store(nullptr);

        std::shared_ptr<T> res;
        if (old_head)
        {
            res.swap(old_head->m_data);
            if (outstanding_hazard_pointers_for(old_head))
            {
                reclaim_later(old_head);
            }
            else
            {
                delete old_head;
            }

            if (nodes_to_reclaim_count.load(std::memory_order_acquire) >= 2 * 100)
            {
                bool expected = false;
                // acquire/release with delete_nodes_with_no_hazards store()
                if (reclamation_in_progress.compare_exchange_strong(expected, true, std::memory_order_acquire,
                                                                    std::memory_order_relaxed))
                {
                    delete_nodes_with_no_hazards();
                }
            }
        }
        return res;
    }
};
