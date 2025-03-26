#include "uthread.h"
#include <stdio.h>

thread_spinlock_t spinlock = {0};
thread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void thread_function(void *arg) {
    int *id = (int *)arg;

    // Test Spinlock
    thread_lock(&spinlock);
    printf("Thread %d: Acquired spinlock\n", *id);
    sleep(1);
    thread_unlock(&spinlock);
    printf("Thread %d: Released spinlock\n", *id);

    // Test Mutex
    thread_mutex_lock(&mutex);
    printf("Thread %d: Acquired mutex\n", *id);
    sleep(1);
    thread_mutex_unlock(&mutex);
    printf("Thread %d: Released mutex\n", *id);

    thread_exit();
}

int main() {
    uthread_t thread1, thread2;
    int id1 = 1, id2 = 2;

    printf("Main process ID: %d\n", getpid());

    thread_create(&thread1, thread_function, &id1, 1);
    thread_create(&thread2, thread_function, &id2, 1);

    thread_join(thread1);
    thread_join(thread2);

    printf("All threads finished!\n");
    return 0;
}
