#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#define N 4
#define LEN 64

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t b;

static int buf1[LEN];
static int buf2[LEN];
static int racey = 0;
static int *cur = buf1;

static void *worker(void *arg) {
  intptr_t tid = (intptr_t)arg;

  if (tid == 0) {
    pthread_mutex_lock(&m);
    cur = buf2;
    pthread_mutex_unlock(&m);
  }

  pthread_barrier_wait(&b);

  racey = 0;
  int s = 0;
  for (int i = 0; i < LEN; i++) {
    s += cur[i];
  }

  if (s == 123456789) {
    abort();
  }

  return NULL;
}

int main(void) {
  pthread_t t[N];

  for (int i = 0; i < LEN; i++) {
    buf1[i] = i;
    buf2[i] = LEN - i;
  }

  pthread_barrier_init(&b, NULL, N);

  for (intptr_t i = 0; i < N; i++) {
    pthread_create(&t[i], NULL, worker, (void *)i);
  }

  for (int i = 0; i < N; i++) {
    pthread_join(t[i], NULL);
  }

  pthread_barrier_destroy(&b);
  pthread_mutex_destroy(&m);

  return 0;
}