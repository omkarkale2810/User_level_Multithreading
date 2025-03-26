#ifndef UTHREAD_H
#define UTHREAD_H

#include <sys/types.h>
#include <sys/mman.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <stdatomic.h>
#include <pthread.h>

#define STACK_SIZE (1024 * 1024)  // 1MB stack

typedef int uthread_t;

// Thread API functions
int thread_create(uthread_t *thread, void (*start_routine)(void *), void *arg, int mapping);
void thread_exit();
int thread_join(uthread_t thread);
void thread_cancel(uthread_t thread);

// Spinlock structure
typedef struct {
    atomic_int lock;
} thread_spinlock_t;

void thread_lock(thread_spinlock_t *lock);
void thread_unlock(thread_spinlock_t *lock);

// Mutex
typedef pthread_mutex_t thread_mutex_t;
void thread_mutex_lock(thread_mutex_t *mutex);
void thread_mutex_unlock(thread_mutex_t *mutex);

#endif
