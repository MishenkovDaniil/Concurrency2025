#ifndef SPIN_LOCK_TAS_H
#define SPIN_LOCK_TAS_H

#include <stdatomic.h>

typedef struct spinlock_tas
{
    atomic_uint m_spin;
} spinlock_tas_t;

spinlock_tas_t *spinlock_tas_ctor();
void spinlock_tas_dtor(spinlock_tas_t *);

void spinlock_tas_lock(void *);
void spinlock_tas_unlock(void *);

#endif /* SPIN_LOCK_TAS_H */
