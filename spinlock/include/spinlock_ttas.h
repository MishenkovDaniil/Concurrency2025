#ifndef SPIN_LOCK_TTAS_H
#define SPIN_LOCK_TTAS_H

#include <stdatomic.h>

typedef struct spinlock_ttas
{
    atomic_uint m_spin;
} spinlock_ttas_t;

spinlock_ttas_t *spinlock_ttas_ctor();
void spinlock_ttas_dtor(spinlock_ttas_t *);

void spinlock_ttas_lock(void *);
void spinlock_ttas_unlock(void *);


typedef struct spinlock_ttas_optimal
{
    atomic_uint m_spin;
} spinlock_ttas_optimal_t;

spinlock_ttas_optimal_t *spinlock_ttas_optimal_ctor();
void spinlock_ttas_optimal_dtor(spinlock_ttas_optimal_t *);

void spinlock_ttas_optimal_lock(void *);
void spinlock_ttas_optimal_unlock(void *);



#endif /* SPIN_LOCK_TTAS_H */
