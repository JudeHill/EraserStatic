#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 4
static pthread_t tids[NUM_THREADS];
static int count = 0;
pthread_mutex_t mutex_1;
pthread_mutex_t mutex_2;
pthread_barrier_t barrier;

void *worker(void *arg) {
  pthread_mutex_lock(&mutex_1);
  count++;
  pthread_mutex_unlock(&mutex_1);
  pthread_barrier_wait(&barrier);
  printf(count);
}

int main(void) {
  pthread_t threads[NUM_THREADS];
  count = 10;
  pthread_mutex_init(&mutex_1, NULL);
  pthread_barrier_init(&barrier, NULL, NUM_THREADS);
  for (int i = 0; i < NUM_THREADS;i++) {
    int *arg = malloc(sizeof(int));
    *arg = i;
    int Error = pthread_create(&threads[i], NULL, (void *(*)(void *))(worker), NULL);
    // pthread_create(&threads[i], NULL, (void * (*))(print_tid), NULL);
  }
  

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
    count++;
  }
  count = 6;
  pthread_barrier_destroy(&barrier);

  return 0;
}