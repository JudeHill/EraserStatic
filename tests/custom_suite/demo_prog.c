#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 4

static pthread_t threads[NUM_THREADS];
static int tids[NUM_THREADS];
static int data[NUM_THREADS];

static int count = 0;
static int a, b;

pthread_mutex_t mutex_1;
pthread_mutex_t mutex_2;
pthread_barrier_t barrier;

void *worker(void *arg) {
  int tid = *((int *)arg);

  data[tid] = tid;

  pthread_mutex_lock(&mutex_1);
  a = 2;
  count++;
  pthread_mutex_unlock(&mutex_1);

  b = 5;

  pthread_barrier_wait(&barrier);

  printf("%d\n", count);

  return NULL;
}

int main(void) {
  count = 10;

  pthread_mutex_init(&mutex_1, NULL);
  pthread_mutex_init(&mutex_2, NULL);
  pthread_barrier_init(&barrier, NULL, NUM_THREADS);

  for (int i = 0; i < NUM_THREADS; i++) {
    tids[i] = i;
    a = 1;

    int error = pthread_create(&threads[i], NULL, worker, &tids[i]);
    if (error != 0) {
      fprintf(stderr, "pthread_create failed\n");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  pthread_barrier_destroy(&barrier);
  pthread_mutex_destroy(&mutex_1);
  pthread_mutex_destroy(&mutex_2);

  return 0;
}