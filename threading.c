#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#define THREAD_READY    0
#define THREAD_RUNNING  1
#define THREAD_BLOCKED  2
#define THREAD_FINISHED 3

#define STACK_SIZE (64 * 1024) 

#define MAX_THREADS 128

typedef struct {
    pthread_t kernel_tid;       
    ucontext_t context;         
    void *stack;                
    int state;                  
    int thread_id;              
    void *(*start_routine)(void *); 
    void *arg;                  
    void *return_value;         
    int joined_by;              
} thread_t;

static thread_t thread_table[MAX_THREADS];
static int thread_count = 0;
static int current_thread = -1;
static int next_thread_id = 0;

void thread_init(void) {
    thread_table[0].thread_id = next_thread_id++;
    thread_table[0].state = THREAD_RUNNING;
    thread_table[0].joined_by = -1;
    thread_table[0].kernel_tid = pthread_self();
    current_thread = 0;
    thread_count = 1;
}

static void *thread_wrapper(void *arg) {
    thread_t *thread = (thread_t *)arg;
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

int thread_create(int *thread_id, void *(*start_routine)(void *), void *arg) {
    if (thread_count >= MAX_THREADS) {
        return -1;  
    }
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        if (i != current_thread && thread_table[i].state == THREAD_FINISHED) {
            break;
        }
    }
    if (i == MAX_THREADS) {
        for (i = 0; i < MAX_THREADS; i++) {
            if (thread_table[i].thread_id == 0 && i != 0) {
                break;
            }
        }
    }
    if (i == MAX_THREADS) {
        return -1;  
    }
    
    thread_table[i].thread_id = next_thread_id++;
    thread_table[i].state = THREAD_READY;
    thread_table[i].start_routine = start_routine;
    thread_table[i].arg = arg;
    thread_table[i].joined_by = -1;
    
    if (pthread_create(&thread_table[i].kernel_tid, NULL, thread_wrapper, &thread_table[i]) != 0) {
        thread_table[i].thread_id = 0;
        return -1;
    }
    
    *thread_id = thread_table[i].thread_id;
    thread_count++;
    
    return 0;
}

void thread_yield(void) {
    sched_yield();
}

int thread_join(int thread_id, void **return_value) {
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].thread_id == thread_id) {
            break;
        }
    }
    if (i == MAX_THREADS) {
        return -1;
    }
    if (thread_table[i].state == THREAD_FINISHED) {
        if (return_value != NULL) {
            *return_value = thread_table[i].return_value;
        }
        return 0;
    }
    thread_table[i].joined_by = current_thread;

    void *temp_retval;
    pthread_join(thread_table[i].kernel_tid, &temp_retval);
    
    if (return_value != NULL) {
        *return_value = thread_table[i].return_value;
    }
    return 0;
}

void thread_exit(void *return_value) {
    if (current_thread >= 0) {
        thread_table[current_thread].return_value = return_value;
        thread_table[current_thread].state = THREAD_FINISHED;

        if (thread_table[current_thread].joined_by >= 0) {
            thread_table[thread_table[current_thread].joined_by].state = THREAD_READY;
        }
        
        thread_count--;
    }
    pthread_exit(NULL);
}

int thread_self(void) {
    return (current_thread >= 0) ? thread_table[current_thread].thread_id : -1;
}

// mutex implementation

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

// Condition Variable Implementation 

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
