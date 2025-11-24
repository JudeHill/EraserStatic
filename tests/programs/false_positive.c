
#include <pthread.h>
#include <stdio.h>

#define NTHREADS 4
#define ITERS    1000000

// Written once in main, then read by workers.
static int config_value;

void* reader(void* arg) {
    (void)arg;
    unsigned long sum = 0;

    // Read-only access (no locks).
    for (int i = 0; i < ITERS; i++) {
        sum += config_value;
    }

    printf("sum=%lu\n", sum); 
    return NULL;
}

int main(void) {
    // Single-threaded initialization.
    config_value = 42;

    pthread_t th[NTHREADS];
    for (int i = 0; i < NTHREADS; i++) {
        pthread_create(&th[i], NULL, reader, NULL);
    }
    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(th[i], NULL);
    }
    return 0;
}