#pragma once

#include <atomic>
#include <stdexcept>
#include <thread>

unsigned const max_hazard_pointers = 100;
struct hazard_pointer
{
    std::atomic<std::thread::id> m_id;
    std::atomic<void *> m_pointer;
};

extern hazard_pointer g_hazard_pointers[max_hazard_pointers];
class hp_owner
{
    hazard_pointer *m_hp;

  public:
    hp_owner(hp_owner const &) = delete;
    hp_owner &operator=(hp_owner const &) = delete;

    hp_owner() : m_hp(nullptr)
    {
        for (unsigned i = 0; i < max_hazard_pointers; ++i)
        {
            std::thread::id old_id;
            if (g_hazard_pointers[i].m_id.compare_exchange_strong(old_id, std::this_thread::get_id(),
                                                                  std::memory_order_release, std::memory_order_relaxed))
            {
                m_hp = &g_hazard_pointers[i];
                break;
            }
        }
        if (!m_hp)
        {
            throw std::runtime_error("No hazard pointers available");
        }
    }

    std::atomic<void *> &get_pointer()
    {
        return m_hp->m_pointer;
    }

    ~hp_owner()
    {
        m_hp->m_pointer.store(nullptr, std::memory_order_relaxed);
        m_hp->m_id.store(std::thread::id(), std::memory_order_release);
    }
};

std::atomic<void *> &get_hazard_pointer_for_current_thread();
bool outstanding_hazard_pointers_for(void *p);