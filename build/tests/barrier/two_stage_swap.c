#define _XOPEN_SOURCE 600

#include <pthread.h>
#include <stdlib.h>
#define NUM_THREADS 4

static pthread_t tids[NUM_THREADS];
static int a = 0;
static int b = 0;
pthread_mutex_t mutex_1;
pthread_mutex_t mutex_2;
pthread_barrier_t b1, b2;

void *worker_1(void *arg) {
  pthread_mutex_lock(&mutex_1);
  a++;
  pthread_mutex_unlock(&mutex_1);
  pthread_barrier_wait(&b1);

  // wait for main thread

  pthread_barrier_wait(&b2);
  pthread_mutex_lock(&mutex_1);
  b++;
  pthread_mutex_unlock(&mutex_1);
}

void *worker_2(void *arg) {
    
    // wait for main thread
    pthread_mutex_lock(&mutex_2);
    b++;
    pthread_mutex_unlock(&mutex_2);
  
    pthread_barrier_wait(&b2);
    
    pthread_mutex_lock(&mutex_2);
    a++;
    pthread_mutex_unlock(&mutex_2);
    
  }

int main(void) {
  pthread_t threads_1[NUM_THREADS];
  pthread_t threads_2[NUM_THREADS];
  pthread_mutex_init(&mutex_1, NULL);
  pthread_barrier_init(&b1, NULL, NUM_THREADS + 1);
  for (int i = 0; i < NUM_THREADS; i++) {
    int *arg = malloc(sizeof(int));
    *arg = i;
    pthread_create(&threads_1[i], NULL, (void *(*)(void *))(worker_1), NULL);
    pthread_create(&threads_2[i], NULL, (void * (*))(worker_2), NULL);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads_1[i], NULL);
    pthread_join(threads_2[i], NULL);
  }
  pthread_barrier_destroy(&b1);

  return 0;
}