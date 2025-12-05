#include "spinlock_ttas.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sched.h>

spinlock_ttas_t *spinlock_ttas_ctor()
{
    spinlock_ttas_t *slock = (spinlock_ttas_t *)malloc(sizeof(spinlock_ttas_t) * 1);
    assert(slock);

    slock->m_spin = 0;
    return slock;
}

void spinlock_ttas_dtor(spinlock_ttas_t * slock)
{
    free(slock);
}

void spinlock_ttas_lock(void *void_slock)
{
    assert(void_slock);
    spinlock_ttas_t *slock = (spinlock_ttas_t *)void_slock;

    uint val = 0;
    do
    {
        // accept false-positive
        while(atomic_load_explicit(&slock->m_spin, memory_order_relaxed))
            ;
        val = 0;
    }
    // same as in TAS: while loop with load doesn't matter at all
    while(!atomic_compare_exchange_weak_explicit(&slock->m_spin, &val, 1, memory_order_release, memory_order_acquire));
}

void spinlock_ttas_unlock(void *void_slock)
{
    assert(void_slock);
    spinlock_ttas_t *slock = (spinlock_ttas_t *)void_slock;

    // since we have CAS in while(1) loop, we don't need to force threads to see changes
    atomic_store_explicit(&slock->m_spin, 0, memory_order_relaxed);
}


spinlock_ttas_optimal_t *spinlock_ttas_optimal_ctor()
{
    spinlock_ttas_optimal_t *slock = (spinlock_ttas_optimal_t *)malloc(sizeof(spinlock_ttas_optimal_t) * 1);
    assert(slock);

    slock->m_spin = 0;
    return slock;
}

void spinlock_ttas_optimal_dtor(spinlock_ttas_optimal_t * slock)
{
    free(slock);
}

void spinlock_ttas_optimal_lock(void *void_slock)
{
    assert(void_slock);
    spinlock_ttas_optimal_t *slock = (spinlock_ttas_optimal_t *)void_slock;

    uint val = 0;
    do
    {
        uint backoff = 1;
        // accept false-positive
        while(atomic_load_explicit(&slock->m_spin, memory_order_relaxed))
        {
            usleep(backoff + rand() % 1000);
            backoff = backoff < 500 ? backoff * 2 : 1000;
        }
        val = 0;
    }
    // same as in TAS: while loop with load doesn't matter at all
    while(!atomic_compare_exchange_weak_explicit(&slock->m_spin, &val, 1, memory_order_release, memory_order_acquire));
}

void spinlock_ttas_optimal_unlock(void *void_slock)
{
    assert(void_slock);
    spinlock_ttas_optimal_t *slock = (spinlock_ttas_optimal_t *)void_slock;

    // since we have CAS in while(1) loop, we don't need to force threads to see changes
    atomic_store_explicit(&slock->m_spin, 0, memory_order_relaxed);
}


spinlock_ttas_yield_t *spinlock_ttas_yield_ctor()
{
    spinlock_ttas_yield_t *slock = (spinlock_ttas_yield_t *)malloc(sizeof(spinlock_ttas_yield_t) * 1);
    assert(slock);

    slock->m_spin = 0;
    return slock;
}

void spinlock_ttas_yield_dtor(spinlock_ttas_yield_t * slock)
{
    free(slock);
}

void spinlock_ttas_yield_lock(void *void_slock)
{
    assert(void_slock);
    spinlock_ttas_yield_t *slock = (spinlock_ttas_yield_t *)void_slock;

    uint val = 0;
    // same as in TAS: while loop with load doesn't matter at all
    while(!atomic_compare_exchange_weak_explicit(&slock->m_spin, &val, 1, memory_order_release, memory_order_acquire));
    {
        while(atomic_load_explicit(&slock->m_spin, memory_order_relaxed))
        {
            // yield hoping that scheduler will woke us in good time
            sched_yield();
        }
        val = 0;
    }
}

void spinlock_ttas_yield_unlock(void *void_slock)
{
    assert(void_slock);
    spinlock_ttas_yield_t *slock = (spinlock_ttas_yield_t *)void_slock;

    // since we have CAS in while(1) loop, we don't need to force threads to see changes
    atomic_store_explicit(&slock->m_spin, 0, memory_order_relaxed);
}


spinlock_ttas_nop_t *spinlock_ttas_nop_ctor()
{
    spinlock_ttas_nop_t *slock = (spinlock_ttas_nop_t *)malloc(sizeof(spinlock_ttas_nop_t) * 1);
    assert(slock);

    slock->m_spin = 0;
    return slock;
}

void spinlock_ttas_nop_dtor(spinlock_ttas_nop_t * slock)
{
    free(slock);
}

void spinlock_ttas_nop_lock(void *void_slock)
{
    assert(void_slock);
    spinlock_ttas_nop_t *slock = (spinlock_ttas_nop_t *)void_slock;

    uint val = 0;
    // same as in TAS: while loop with load doesn't matter at all
    while(!atomic_compare_exchange_weak_explicit(&slock->m_spin, &val, 1, memory_order_release, memory_order_acquire));
    {
        while(atomic_load_explicit(&slock->m_spin, memory_order_relaxed))
        {
            // sleep but not saying to scheduler about it)
            __asm__ __volatile__(
                "nop\n\t"
                "nop\n\t"
                "nop\n\t"
            );
        }
        val = 0;
    }
}

void spinlock_ttas_nop_unlock(void *void_slock)
{
    assert(void_slock);
    spinlock_ttas_nop_t *slock = (spinlock_ttas_nop_t *)void_slock;

    // since we have CAS in while(1) loop, we don't need to force threads to see changes
    atomic_store_explicit(&slock->m_spin, 0, memory_order_relaxed);
}