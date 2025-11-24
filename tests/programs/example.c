#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#define NTHREADS 4
#define INCREMENTS 100000

static long counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock_2 = PTHREAD_MUTEX_INITIALIZER;


void* worker(void* arg) {
    (void)arg; // unused
    for (int i = 0; i < INCREMENTS; i++) {
        // pthread_mutex_lock(&lock);
        counter++;
        // pthread_mutex_unlock(&lock);
    }
}


int main(void) {
    pthread_t threads[NTHREADS];

    // Create threads
    for (int i = 0; i < NTHREADS; i++) {
        if (pthread_create(&threads[i], NULL, worker, NULL) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    // Wait for them to finish
    for (int i = 0; i < NTHREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }

    printf("Expected: %ld\n", (long)NTHREADS * INCREMENTS);
    printf("Actual:   %ld\n", counter);

    // Clean up the mutex (optional here since program is exiting)
    pthread_mutex_destroy(&lock);

    return 0;
}
