#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Number of threads
#define NUM_THREADS 4

// Function executed by each thread
void* print_message(void* thread_id) {
    long tid = (long) thread_id;
    printf("Hello from thread %ld!\n", tid);
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    int rc;
    long t;

    for (t = 0; t < NUM_THREADS; t++) {
        printf("Creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, print_message, (void*) t);
        if (rc) {
            fprintf(stderr, "Error: unable to create thread, %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to finish
    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    printf("All threads completed.\n");
    pthread_exit(NULL);
}
