
#ifndef THREADING_H
#define THREADING_H

#define THREAD_READY    0
#define THREAD_RUNNING  1
#define THREAD_BLOCKED  2
#define THREAD_FINISHED 3

void thread_init(void);

int thread_create(int *thread_id, void *(*start_routine)(void *), void *arg);

void thread_yield(void);

int thread_join(int thread_id, void **return_value);

void thread_exit(void *return_value);

int thread_self(void);


typedef struct {
    pthread_mutex_t mutex;
    int initialized;
} thread_mutex_t;

int thread_mutex_init(thread_mutex_t *mutex);
int thread_mutex_lock(thread_mutex_t *mutex);
int thread_mutex_unlock(thread_mutex_t *mutex);
int thread_mutex_destroy(thread_mutex_t *mutex);


typedef struct {
    pthread_cond_t cond;
    int initialized;
} thread_cond_t;

int thread_cond_init(thread_cond_t *cond);
int thread_cond_wait(thread_cond_t *cond, thread_mutex_t *mutex);
int thread_cond_signal(thread_cond_t *cond);
int thread_cond_broadcast(thread_cond_t *cond);
int thread_cond_destroy(thread_cond_t *cond);

#endif 
