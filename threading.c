/*
 * User-level One-to-One Threading Library
 * 
 * This library implements a user-level threading system where each user thread
 * maps to one kernel thread (one-to-one model).
 */

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

/* Thread States */
#define THREAD_READY    0
#define THREAD_RUNNING  1
#define THREAD_BLOCKED  2
#define THREAD_FINISHED 3

/* Thread Stack Size */
#define STACK_SIZE (64 * 1024)  /* 64KB stack */

/* Maximum number of threads */
#define MAX_THREADS 128

/* Thread Control Block */
typedef struct {
    pthread_t kernel_tid;       /* Kernel thread ID */
    ucontext_t context;         /* Thread context */
    void *stack;                /* Thread stack */
    int state;                  /* Thread state */
    int thread_id;              /* User-level thread ID */
    void *(*start_routine)(void *); /* Thread start function */
    void *arg;                  /* Argument to start function */
    void *return_value;         /* Return value from thread */
    int joined_by;              /* ID of thread waiting for this thread */
} thread_t;

/* Thread table to keep track of all threads */
static thread_t thread_table[MAX_THREADS];

/* Number of active threads */
static int thread_count = 0;

/* Current running thread ID */
static int current_thread = -1;

/* Next available thread ID */
static int next_thread_id = 0;

/* Initialize the threading library */
void thread_init(void) {
    /* Initialize the main thread (thread 0) */
    thread_table[0].thread_id = next_thread_id++;
    thread_table[0].state = THREAD_RUNNING;
    thread_table[0].joined_by = -1;
    thread_table[0].kernel_tid = pthread_self();
    current_thread = 0;
    thread_count = 1;
}

/* Thread wrapper function that calls the actual thread function */
static void *thread_wrapper(void *arg) {
    thread_t *thread = (thread_t *)arg;
    
    /* Execute the thread function */
    thread->return_value = thread->start_routine(thread->arg);
    
    /* Mark thread as finished */
    thread->state = THREAD_FINISHED;
    
    /* Wake up any thread waiting for this one to finish */
    if (thread->joined_by >= 0) {
        thread_table[thread->joined_by].state = THREAD_READY;
    }
    
    pthread_exit(NULL);
    return NULL;  /* Never reached */
}

/* Create a new thread */
int thread_create(int *thread_id, void *(*start_routine)(void *), void *arg) {
    if (thread_count >= MAX_THREADS) {
        return -1;  /* Too many threads */
    }
    
    /* Find an empty slot in the thread table */
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        if (i != current_thread && thread_table[i].state == THREAD_FINISHED) {
            break;
        }
    }
    
    if (i == MAX_THREADS) {
        /* No empty slots found, find the first unused slot */
        for (i = 0; i < MAX_THREADS; i++) {
            if (thread_table[i].thread_id == 0 && i != 0) {
                break;
            }
        }
    }
    
    if (i == MAX_THREADS) {
        return -1;  /* No available slots */
    }
    
    /* Initialize the thread control block */
    thread_table[i].thread_id = next_thread_id++;
    thread_table[i].state = THREAD_READY;
    thread_table[i].start_routine = start_routine;
    thread_table[i].arg = arg;
    thread_table[i].joined_by = -1;
    
    /* Create the kernel thread (pthread) for one-to-one mapping */
    if (pthread_create(&thread_table[i].kernel_tid, NULL, thread_wrapper, &thread_table[i]) != 0) {
        thread_table[i].thread_id = 0;
        return -1;
    }
    
    *thread_id = thread_table[i].thread_id;
    thread_count++;
    
    return 0;
}

/* Yield execution to another thread */
void thread_yield(void) {
    /* In a one-to-one model, we just yield to the kernel scheduler */
    sched_yield();
}

/* Wait for a thread to finish */
int thread_join(int thread_id, void **return_value) {
    int i;
    
    /* Find the thread with given ID */
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].thread_id == thread_id) {
            break;
        }
    }
    
    if (i == MAX_THREADS) {
        return -1;  /* Thread not found */
    }
    
    /* If thread already finished, just get its return value */
    if (thread_table[i].state == THREAD_FINISHED) {
        if (return_value != NULL) {
            *return_value = thread_table[i].return_value;
        }
        return 0;
    }
    
    /* Mark that we're waiting for this thread */
    thread_table[i].joined_by = current_thread;
    
    /* Wait for the thread to complete using pthread_join */
    void *temp_retval;
    pthread_join(thread_table[i].kernel_tid, &temp_retval);
    
    /* Get the return value */
    if (return_value != NULL) {
        *return_value = thread_table[i].return_value;
    }
    
    return 0;
}

/* Exit the current thread */
void thread_exit(void *return_value) {
    if (current_thread >= 0) {
        thread_table[current_thread].return_value = return_value;
        thread_table[current_thread].state = THREAD_FINISHED;
        
        /* Wake up any thread waiting for this one to finish */
        if (thread_table[current_thread].joined_by >= 0) {
            thread_table[thread_table[current_thread].joined_by].state = THREAD_READY;
        }
        
        thread_count--;
    }
    
    pthread_exit(NULL);  /* Exit the kernel thread */
}

/* Get current thread ID */
int thread_self(void) {
    return (current_thread >= 0) ? thread_table[current_thread].thread_id : -1;
}

/* ============= Mutex Implementation ============= */

typedef struct {
    pthread_mutex_t mutex;
    int initialized;
} thread_mutex_t;

int thread_mutex_init(thread_mutex_t *mutex) {
    int result = pthread_mutex_init(&mutex->mutex, NULL);
    if (result == 0) {
        mutex->initialized = 1;
    }
    return result;
}

int thread_mutex_lock(thread_mutex_t *mutex) {
    if (!mutex->initialized) {
        return -1;
    }
    return pthread_mutex_lock(&mutex->mutex);
}

int thread_mutex_unlock(thread_mutex_t *mutex) {
    if (!mutex->initialized) {
        return -1;
    }
    return pthread_mutex_unlock(&mutex->mutex);
}

int thread_mutex_destroy(thread_mutex_t *mutex) {
    if (!mutex->initialized) {
        return -1;
    }
    int result = pthread_mutex_destroy(&mutex->mutex);
    if (result == 0) {
        mutex->initialized = 0;
    }
    return result;
}

/* ============= Condition Variable Implementation ============= */

typedef struct {
    pthread_cond_t cond;
    int initialized;
} thread_cond_t;

int thread_cond_init(thread_cond_t *cond) {
    int result = pthread_cond_init(&cond->cond, NULL);
    if (result == 0) {
        cond->initialized = 1;
    }
    return result;
}

int thread_cond_wait(thread_cond_t *cond, thread_mutex_t *mutex) {
    if (!cond->initialized || !mutex->initialized) {
        return -1;
    }
    return pthread_cond_wait(&cond->cond, &mutex->mutex);
}

int thread_cond_signal(thread_cond_t *cond) {
    if (!cond->initialized) {
        return -1;
    }
    return pthread_cond_signal(&cond->cond);
}

int thread_cond_broadcast(thread_cond_t *cond) {
    if (!cond->initialized) {
        return -1;
    }
    return pthread_cond_broadcast(&cond->cond);
}

int thread_cond_destroy(thread_cond_t *cond) {
    if (!cond->initialized) {
        return -1;
    }
    int result = pthread_cond_destroy(&cond->cond);
    if (result == 0) {
        cond->initialized = 0;
    }
    return result;
}
