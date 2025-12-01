#include "ticket_lock.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

ticket_lock_t *ticket_lock_ctor()
{
    ticket_lock_t *tlock = (ticket_lock_t *)malloc(sizeof(ticket_lock_t) * 1);
    assert(tlock);

    tlock->cur = 0;
    tlock->next = 0;
    return tlock;
}

void ticket_lock_dtor(ticket_lock_t * tlock)
{
    free(tlock);
}

void ticket_lock_lock(void *void_tlock)
{
    assert(void_tlock);
    ticket_lock_t *tlock = (ticket_lock_t *)void_tlock;

    // we don't mind the sequence of fetch-adds
    uint ticket = atomic_fetch_add_explicit(&tlock->next, 1, memory_order_relaxed);

    // acquire/release
    // seems we can use relaxed but it will cause degradation of next-to-lock-thread (and others too)
    while(atomic_load_explicit(&tlock->cur, memory_order_acquire) != ticket)
        ;
}

void ticket_lock_unlock(void *void_tlock)
{
    assert(void_tlock);
    ticket_lock_t *tlock = (ticket_lock_t *)void_tlock;

    // acquire/release
    // seems we can use relaxed but it will cause degradation of next-to-lock-thread (and others too)
    atomic_fetch_add_explicit(&tlock->cur, 1, memory_order_release);
}


ticket_lock_optimal_t *ticket_lock_optimal_ctor()
{
    ticket_lock_optimal_t *tlock = (ticket_lock_optimal_t *)malloc(sizeof(ticket_lock_optimal_t) * 1);
    assert(tlock);

    tlock->cur = 0;
    tlock->next = 0;
    return tlock;
}

void ticket_lock_optimal_dtor(ticket_lock_optimal_t * tlock)
{
    free(tlock);
}

void ticket_lock_optimal_lock(void *void_tlock)
{
    assert(void_tlock);
    ticket_lock_optimal_t *tlock = (ticket_lock_optimal_t *)void_tlock;

    // we don't mind the sequence of fetch-adds
    uint ticket = atomic_fetch_add_explicit(&tlock->next, 1, memory_order_relaxed);
    uint backoff = 1;

    // acquire/release
    // seems we can use relaxed but it will cause degradation of next-to-lock-thread (and others too)
    while(atomic_load_explicit(&tlock->cur, memory_order_acquire) != ticket)
    {
        usleep(backoff + rand() % 1000);
        backoff = backoff < 500 ? backoff * 2 : 1000;
    }
}

void ticket_lock_optimal_unlock(void *void_tlock)
{
    assert(void_tlock);
    ticket_lock_optimal_t *tlock = (ticket_lock_optimal_t *)void_tlock;

    // acquire/release
    // seems we can use relaxed but it will cause degradation of next-to-lock-thread (and others too)
    atomic_fetch_add_explicit(&tlock->cur, 1, memory_order_release);
}