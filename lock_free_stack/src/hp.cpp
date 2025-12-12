#include "hp.hpp"
#include <atomic>
#include <iostream>
#include <memory>

// global hazard pointers array
hazard_pointer g_hazard_pointers[max_hazard_pointers];

std::atomic<void *> &get_hazard_pointer_for_current_thread()
{
    thread_local static hp_owner hazard;
    return hazard.get_pointer();
}

bool outstanding_hazard_pointers_for(void *p)
{
    for (unsigned i = 0; i < max_hazard_pointers; ++i)
    {
        if (g_hazard_pointers[i].m_pointer.load(std::memory_order_acquire) == p)
        {
            return true;
        }
    }
    return false;
}