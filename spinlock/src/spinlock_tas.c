#include "spinlock_tas.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <assert.h>

spinlock_tas_t *spinlock_tas_ctor()
{
    spinlock_tas_t *slock = (spinlock_tas_t *)malloc(sizeof(spinlock_tas_t) * 1);
    assert(slock);

    slock->m_spin = 0;
    return slock;
}

void spinlock_tas_dtor(spinlock_tas_t * slock)
{
    free(slock);
}

void spinlock_tas_lock(void *void_slock)
{
    assert(void_slock);
    spinlock_tas_t *slock = (spinlock_tas_t *)void_slock;

    uint val;
    while(1)
    {
        val = 0;
        // when one thread makes cas, others must see 0->1, to avoid double lock. this why we need acquire/release lock
        if (atomic_compare_exchange_weak_explicit(&slock->m_spin, &val, 1, memory_order_release, memory_order_acquire))
            break;
    }
}

void spinlock_tas_unlock(void *void_slock)
{
    assert(void_slock);
    spinlock_tas_t *slock = (spinlock_tas_t *)void_slock;

    // since we have CAS in while(1) loop, we don't need to force threads to see changes
    atomic_store_explicit(&slock->m_spin, 0, memory_order_relaxed);
}
