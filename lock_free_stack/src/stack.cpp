#include <stdlib.h>

#include "stack.hpp"

std::atomic<data_to_reclaim_base *> nodes_to_reclaim(nullptr);
std::atomic<size_t> nodes_to_reclaim_count(0);
std::atomic<bool> reclamation_in_progress(false);

template <typename T> void do_delete(void *p)
{
    delete static_cast<T *>(p);
}

void add_to_reclaim_list(data_to_reclaim_base *node)
{
    node->m_next = nodes_to_reclaim.load(std::memory_order_relaxed);
    // acquire/release with exchange
    while (!nodes_to_reclaim.compare_exchange_weak(node->m_next, node, std::memory_order_release,
                                                   std::memory_order_relaxed))
        ;
}

void delete_nodes_with_no_hazards()
{
    // acquire/release with CAS loop
    data_to_reclaim_base *current = nodes_to_reclaim.exchange(nullptr, std::memory_order_acquire);
    while (current)
    {
        data_to_reclaim_base *const next = current->m_next;
        if (!outstanding_hazard_pointers_for(current->m_data))
        {
            nodes_to_reclaim_count.fetch_sub(1, std::memory_order_release);
            current->m_deleter(current->m_data);
            delete current;
        }
        else
        {
            add_to_reclaim_list(current);
        }
        current = next;
    }
    reclamation_in_progress.store(false, std::memory_order_release);
}