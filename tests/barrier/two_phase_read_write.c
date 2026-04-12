// barrier_double_buffer.c
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_barrier_t B;
static pthread_mutex_t Lw1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t Lw2 = PTHREAD_MUTEX_INITIALIZER;

static int BUF1 = 0;
static int BUF2 = 0;

static void *worker(void *arg) {
  (void)arg;

  // Phase 1: write BUF1 under Lw1, read BUF2 without lock (read-only in this phase)
  pthread_mutex_lock(&Lw1);
  BUF1 += 1;
  pthread_mutex_unlock(&Lw1);

  int r1 = BUF2; (void)r1;

  pthread_barrier_wait(&B);

  // Phase 2: write BUF2 under Lw2, read BUF1 without lock (read-only in this phase)
  pthread_mutex_lock(&Lw2);
  BUF2 += 1;
  pthread_mutex_unlock(&Lw2);

  int r2 = BUF1; (void)r2;

  return NULL;
}

int main(int argc, char **argv) {
  int n = (argc > 1) ? atoi(argv[1]) : 4;
  pthread_t *t = malloc(sizeof(*t) * n);

  pthread_barrier_init(&B, NULL, (unsigned)n);
  for (int i = 0; i < n; i++) pthread_create(&t[i], NULL, worker, NULL);
  for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
  pthread_barrier_destroy(&B);

  printf("BUF1=%d BUF2=%d\n", BUF1, BUF2);
  free(t);
}
