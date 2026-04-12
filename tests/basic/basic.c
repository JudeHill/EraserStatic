#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 4
static int tids[NUM_THREADS];
static int output[NUM_THREADS];
static int count = 0;
pthread_mutex_t mutex_1;
pthread_barrier_t b1;

void *print_tid(void *arg) {
  int tid = *((int *)arg);
  output[tid] = tid;
  // pthread_mutex_lock(&mutex_1);
  count++;
  // pthread_mutex_unlock(&mutex_1);
}

int main(void) {
  pthread_t threads[10];
  pthread_mutex_init(&mutex_1, NULL);
  for (int i = 0; i < 10;i++) {
    int Error = pthread_create(&threads[i], NULL, (void *(*)(void *))(print_tid), NULL);
  }

  for (int i = 0; i < 5; i++) {
    pthread_join(threads[i], &tids[i]);
  }
  for (int i = 5; i < 10; i++) {
    pthread_join(threads[i], &tids[i]);
  }
  return 0;
}