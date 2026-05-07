#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 4
static int tids[NUM_THREADS];
static int output[NUM_THREADS];
static int count = 0;
pthread_mutex_t mutex_1;
pthread_barrier_t b1;

void *worker(void *arg) {
  int tid = *((int *)arg);
  output[tid] = tid;
  pthread_mutex_lock(&mutex_1);
  count++;
  pthread_mutex_unlock(&mutex_1);
}

int main(void) { 
  // thread_depth = 0
  pthread_t threads[NUM_THREADS]; 
  pthread_mutex_init(&mutex_1, NULL);
  pthread_barrier_init(&b1, NULL, NUM_THREADS);
  int *arg = malloc(sizeof(int));
  for (int i = 0; i < NUM_THREADS;i++) {
    *arg = i;
    pthread_create(&threads[i], NULL, (void *(*)(void *))(worker), arg); 
    // thread_depth = 1
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], &tids[i]); 
    // thread_depth = 0
  }

  return 0;
}