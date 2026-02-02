#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 4
static pthread_t tids[NUM_THREADS];
static int count = 0;
pthread_mutex_t mutex;
pthread_barrier_t barrier;

void* print_tid(void* arg){
    int tid = *((int*) arg);
    tids[tid] = tid;
    pthread_mutex_lock(&mutex);
    count++;
    pthread_mutex_unlock(&mutex);
}

int main(void){
    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    for (int i=0;i<NUM_THREADS;){
        int* arg = malloc(sizeof(int));
        *arg = i;
        int Error = pthread_create(&threads[i], NULL, (void * (*)(void *))(print_tid), NULL);
        // pthread_create(&threads[i], NULL, (void * (*))(print_tid), NULL);
    }
    pthread_barrier_wait(&barrier);

    for (int i=0;i<NUM_THREADS;i++){
        pthread_join(threads[i], NULL);
        count++;
    }
    pthread_barrier_destroy(&barrier);

    return 0;
}