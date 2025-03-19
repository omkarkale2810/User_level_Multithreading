/*
 * Test program for the user-level threading library
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "threading.h"  // Assume the threading library is in this header

#define NUM_THREADS 5
#define ITERATIONS 3

/* Thread function */
void *thread_function(void *arg) {
    int thread_num = *((int *)arg);
    
    for (int i = 0; i < ITERATIONS; i++) {
        printf("Thread %d: Iteration %d\n", thread_num, i);
        /* Sleep to simulate work */
        usleep(100000);  // 100ms
        /* Yield to let other threads run */
        thread_yield();
    }
    
    printf("Thread %d: Finished\n", thread_num);
    return NULL;
}

/* Mutex test function */
thread_mutex_t mutex;
int shared_counter = 0;

void *mutex_test_function(void *arg) {
    int thread_num = *((int *)arg);
    
    for (int i = 0; i < 10; i++) {
        thread_mutex_lock(&mutex);
        shared_counter++;
        printf("Thread %d: Incremented counter to %d\n", thread_num, shared_counter);
        thread_mutex_unlock(&mutex);
        
        thread_yield();
        usleep(50000);  // 50ms
    }
    
    return NULL;
}

/* Condition variable test */
thread_mutex_t cv_mutex;
thread_cond_t cv;
int ready = 0;

void *producer_function(void *arg) {
    printf("Producer: Starting\n");
    
    sleep(2);  // Simulate work
    
    thread_mutex_lock(&cv_mutex);
    ready = 1;
    printf("Producer: Data is ready, signaling consumer\n");
    thread_cond_signal(&cv);
    thread_mutex_unlock(&cv_mutex);
    
    return NULL;
}

void *consumer_function(void *arg) {
    printf("Consumer: Starting\n");
    
    thread_mutex_lock(&cv_mutex);
    while (!ready) {
        printf("Consumer: Waiting for data to be ready\n");
        thread_cond_wait(&cv, &cv_mutex);
    }
    printf("Consumer: Got signal that data is ready\n");
    thread_mutex_unlock(&cv_mutex);
    
    return NULL;
}

int main() {
    /* Initialize the threading library */
    thread_init();
    
    printf("===== Basic Threading Test =====\n");
    
    int thread_ids[NUM_THREADS];
    int thread_args[NUM_THREADS];
    
    /* Create multiple threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i + 1;
        thread_create(&thread_ids[i], thread_function, &thread_args[i]);
        printf("Main: Created thread %d\n", thread_ids[i]);
    }
    
    /* Join all threads */
    printf("Main: Waiting for all threads to finish\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_join(thread_ids[i], NULL);
        printf("Main: Thread %d has finished\n", thread_ids[i]);
    }
    
    printf("\n===== Mutex Test =====\n");
    
    /* Initialize mutex */
    thread_mutex_init(&mutex);
    
    /* Create threads for mutex test */
    int mutex_thread_ids[3];
    int mutex_thread_args[3];
    
    for (int i = 0; i < 3; i++) {
        mutex_thread_args[i] = i + 1;
        thread_create(&mutex_thread_ids[i], mutex_test_function, &mutex_thread_args[i]);
    }
    
    /* Join all mutex test threads */
    for (int i = 0; i < 3; i++) {
        thread_join(mutex_thread_ids[i], NULL);
    }
    
    /* Destroy mutex */
    thread_mutex_destroy(&mutex);
    
    printf("Final counter value: %d\n", shared_counter);
    
    printf("\n===== Condition Variable Test =====\n");
    
    /* Initialize mutex and condition variable */
    thread_mutex_init(&cv_mutex);
    thread_cond_init(&cv);
    
    int producer_id, consumer_id;
    thread_create(&consumer_id, consumer_function, NULL);
    thread_create(&producer_id, producer_function, NULL);
    
    thread_join(consumer_id, NULL);
    thread_join(producer_id, NULL);
    
    /* Destroy mutex and condition variable */
    thread_mutex_destroy(&cv_mutex);
    thread_cond_destroy(&cv);
    
    printf("Main: All tests completed\n");
    
    return 0;
}
