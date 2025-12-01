#ifndef TICKET_LOCK_H
#define TICKET_LOCK_H

#include <stdatomic.h>

typedef struct ticket_lock
{
    atomic_uint cur;
    atomic_uint next;
} ticket_lock_t;

ticket_lock_t *ticket_lock_ctor();
void ticket_lock_dtor(ticket_lock_t *);

void ticket_lock_lock(void *);
void ticket_lock_unlock(void *);


typedef struct ticket_lock_optimal
{
    atomic_uint cur;
    atomic_uint next;
} ticket_lock_optimal_t;

ticket_lock_optimal_t *ticket_lock_optimal_ctor();
void ticket_lock_optimal_dtor(ticket_lock_optimal_t *);

void ticket_lock_optimal_lock(void *);
void ticket_lock_optimal_unlock(void *);

#endif /* TICKET_LOCK_H */
