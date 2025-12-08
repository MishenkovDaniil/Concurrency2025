#include "spinlock_tas.h"
#include "spinlock_ttas.h"
#include "ticket_lock.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

typedef void (*lock_func_t)(void*);
typedef void (*unlock_func_t)(void*);
typedef struct
{
    void *lock_primitive;
    lock_func_t lock_func;
    unlock_func_t unlock_func;
} lock_data_t;

typedef struct
{
    lock_data_t *lock_data;
    double *max;
    double *avg;
} data_t;

double timespec_diff_us(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1e6 +
           (end->tv_nsec - start->tv_nsec) / 1e3;
}

void *worker(void *arg)
{
    data_t *data = (data_t *)arg;
    lock_data_t *lock_data = (lock_data_t *)data->lock_data;
    double *max = (double *)data->max;
    double *avg = (double *)data->avg;

    *max = 0;
    *avg = 0;
    struct timespec t_start, t_end;
    for (int i = 0; i < 10; ++i)
    {
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        lock_data->lock_func(lock_data->lock_primitive);
        clock_gettime(CLOCK_MONOTONIC, &t_end);

        double a = timespec_diff_us(&t_start, &t_end);
        *max = (a > *max) ? a : *max;
        *avg += a;

        usleep(100);
        lock_data->unlock_func(lock_data->lock_primitive);
    }
    *avg /= 10.;
    return NULL;
}

void get_times_for_cryt_section(double *avg_time, double *max_time, int thread_num, void *lock_primitive, lock_func_t lock_func, unlock_func_t unlock_func)
{
    assert(avg_time);
    assert(max_time);
    assert(lock_primitive);

    double *avg_times = (double *)malloc(sizeof(double) * thread_num);
    double *max_times = (double *)malloc(sizeof(double) * thread_num);

    pthread_t *threads = (pthread_t *)malloc(thread_num * sizeof(pthread_t));
    data_t *datas = (data_t *)malloc(thread_num * sizeof(data_t));
    lock_data_t lock_data;
    lock_data.lock_primitive = lock_primitive;
    lock_data.lock_func = lock_func;
    lock_data.unlock_func = unlock_func;

    for (int i = 0; i < thread_num; i++)
    {
        datas[i].lock_data = &lock_data;
        datas[i].avg = avg_times + i;
        datas[i].max = max_times + i;
    }

    for (int i = 0; i < thread_num; i++)
        pthread_create(&threads[i], NULL, worker, datas + i);

    for (int i = 0; i < thread_num; i++)
        pthread_join(threads[i], NULL);

    // статистика
    double sum = 0;
    for (int i = 0; i < thread_num; i++) {
        sum += avg_times[i];
    }
    *avg_time = sum / thread_num;

    for (int i = 0; i < thread_num; i++) {
        *max_time = max_times[i] > *max_time ? max_times[i] : *max_time;
    }

    printf("Average wait: %lf us\n", *avg_time);
    printf("Max wait: %lf us\n", *max_time);
}

void spinlock_tas_get_times(double *avg_time, double *max_time, int thread_num)
{
    printf("SPINLOCK TAS\n");
    spinlock_tas_t *slock = spinlock_tas_ctor();
    get_times_for_cryt_section(avg_time, max_time, thread_num, slock, spinlock_tas_lock, spinlock_tas_unlock);
    spinlock_tas_dtor(slock);
}

void spinlock_ttas_get_times(double *avg_time, double *max_time, int thread_num)
{
    printf("SPINLOCK TTAS\n");
    spinlock_ttas_t *slock = spinlock_ttas_ctor();
    get_times_for_cryt_section(avg_time, max_time, thread_num, slock, spinlock_ttas_lock, spinlock_ttas_unlock);
    spinlock_ttas_dtor(slock);
}

void spinlock_ttas_optimal_get_times(double *avg_time, double *max_time, int thread_num)
{
    printf("SPINLOCK TTAS OPTIMAL\n");
    spinlock_ttas_optimal_t *slock = spinlock_ttas_optimal_ctor();
    get_times_for_cryt_section(avg_time, max_time, thread_num, slock, spinlock_ttas_optimal_lock, spinlock_ttas_optimal_unlock);
    spinlock_ttas_optimal_dtor(slock);
}

void ticket_lock_get_times(double *avg_time, double *max_time, int thread_num)
{
    printf("TICKET LOCK\n");
    ticket_lock_t *slock = ticket_lock_ctor();
    get_times_for_cryt_section(avg_time, max_time, thread_num, slock, ticket_lock_lock, ticket_lock_unlock);
    ticket_lock_dtor(slock);
}

void ticket_lock_optimal_get_times(double *avg_time, double *max_time, int thread_num)
{
    printf("TICKET LOCK OPTIMAL\n");
    ticket_lock_optimal_t *slock = ticket_lock_optimal_ctor();
    get_times_for_cryt_section(avg_time, max_time, thread_num, slock, ticket_lock_optimal_lock, ticket_lock_optimal_unlock);
    ticket_lock_optimal_dtor(slock);
}

void get_times(double *avg_time, double *max_time, int thread_num)
{
    spinlock_tas_get_times(avg_time, max_time, thread_num);
    spinlock_ttas_get_times(avg_time, max_time, thread_num);
    spinlock_ttas_optimal_get_times(avg_time, max_time, thread_num);
    ticket_lock_get_times(avg_time, max_time, thread_num);
    ticket_lock_optimal_get_times(avg_time, max_time, thread_num);
}