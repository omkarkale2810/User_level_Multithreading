#include "uthread.h"

// Thread start function
static int thread_start(void *arg) {
    void (**func_and_arg)(void *) = arg;
    void (*start_routine)(void *) = func_and_arg[0];
    void *arg_data = func_and_arg[1];

    start_routine(arg_data);
    thread_exit();
    return 0;
}

// Create a new thread
int thread_create(uthread_t *thread, void (*start_routine)(void *), void *arg, int mapping) {
    void *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    void *func_and_arg[2] = { start_routine, arg };
    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND;

    if (mapping == 1) { 
        flags |= CLONE_THREAD;
    }

    *thread = clone(thread_start, stack + STACK_SIZE, flags, func_and_arg);
    return (*thread == -1) ? -1 : 0;
}

// Exit the thread
void thread_exit() {
    _exit(0);
}

// Join a thread
int thread_join(uthread_t thread) {
    return waitpid(thread, NULL, 0);
}

// Cancel a thread
void thread_cancel(uthread_t thread) {
    kill(thread, SIGKILL);
}

// Spinlock implementation
void thread_lock(thread_spinlock_t *lock) {
    while (atomic_exchange(&lock->lock, 1) == 1);
}

void thread_unlock(thread_spinlock_t *lock) {
    atomic_store(&lock->lock, 0);
}

// Mutex implementation
void thread_mutex_lock(thread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);
}

void thread_mutex_unlock(thread_mutex_t *mutex) {
    pthread_mutex_unlock(mutex);
}
