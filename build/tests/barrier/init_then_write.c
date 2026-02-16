// barrier_init_then_read.c
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_barrier_t B;

static int CONFIG = 0;
static int DATA = 0;

static void *worker(void *arg) {
  (void)arg;

  // Phase 1: all threads cooperatively "initialize"
  // (e.g. last-writer-wins, but still race-free if you ensure single-writer;
  // here we do deterministic init via barrier staging)
  CONFIG = 42; // naive detector flags writes

  pthread_barrier_wait(&B);

  // Phase 2: CONFIG treated as immutable, read-only
  if (CONFIG == 42) DATA++;

  return NULL;
}

int main(int argc, char **argv) {
  int n = (argc > 1) ? atoi(argv[1]) : 4;
  pthread_t *t = malloc(sizeof(*t) * n);

  pthread_barrier_init(&B, NULL, (unsigned)n);
  for (int i = 0; i < n; i++) pthread_create(&t[i], NULL, worker, NULL);
  for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
  pthread_barrier_destroy(&B);

  printf("CONFIG=%d DATA=%d\n", CONFIG, DATA);
  free(t);
}
