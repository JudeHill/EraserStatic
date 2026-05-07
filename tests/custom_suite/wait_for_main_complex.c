#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NUM_THREADS 4
#define N (1<<20)

static float *x;
static float *y;

static pthread_barrier_t phase1_done;   // workers + main
static pthread_barrier_t main_done;     // workers + main

static pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

static double sum_abs = 0.0;        // workers write under m1
static float threshold = 0.0f;      // main writes without lock
static double sum_above = 0.0;      // workers write under m2

static inline float f_abs(float v) {
  return v < 0 ? -v : v;
}

static inline float transform(float v) {
  float t = v * 1.01f;
  t = t - (t * t) * 0.000001f;
  return t;
}

static void *worker(void *arg) {
  intptr_t tid = (intptr_t)arg;

  size_t chunk = (size_t)N / NUM_THREADS;
  size_t lo = (size_t)tid * chunk;
  size_t hi = (tid == NUM_THREADS - 1) ? (size_t)N : lo + chunk;

  /*
   * Phase 1:
   * Workers transform x into y and publish sum_abs under m1.
   */
  double local_sum = 0.0;

  for (size_t i = lo; i < hi; i++) { 
    float v = transform(x[i]);
    y[i] = v;
    local_sum += (double)f_abs(v);
  }

  pthread_mutex_lock(&m1);
  sum_abs += local_sum;
  pthread_mutex_unlock(&m1);

  /*
   * Stop point:
   * all workers have finished Phase 1 before main continues.
   */
  pthread_barrier_wait(&phase1_done);

  /*
   * Main thread now computes threshold and resets sum_abs without locks.
   * Workers wait here until that unlocked work is complete.
   */
  pthread_barrier_wait(&main_done);

  /*
   * Phase 2:
   * Workers read threshold and update sum_above under m2.
   */
  double local_above = 0.0;
  float thr = threshold;

  for (size_t i = lo; i < hi; i++) {
    float v = y[i];

    if (f_abs(v) > thr) {
      local_above += (double)f_abs(v);
    }

    x[i] = v * 0.99f;
  }

  pthread_mutex_lock(&m2);
  sum_above += local_above;
  pthread_mutex_unlock(&m2);

  return NULL;
}

int main(void) {
  pthread_t threads[NUM_THREADS];

  x = (float*)malloc(sizeof(float) * N);
  y = (float*)malloc(sizeof(float) * N);

  if (!x || !y) return 1;

  for (size_t i = 0; i < N; i++) {
    x[i] = (float)(i % 1000) * 0.001f;
  }

  pthread_barrier_init(&phase1_done, NULL, NUM_THREADS + 1);
  pthread_barrier_init(&main_done, NULL, NUM_THREADS + 1);

  pthread_mutex_init(&m1, NULL);
  pthread_mutex_init(&m2, NULL);

  for (intptr_t t = 0; t < NUM_THREADS; t++) {
    pthread_create(&threads[t], NULL, worker, (void*)t);
  }

  /*
   * Wait until all workers have updated sum_abs under m1.
   */
  pthread_barrier_wait(&phase1_done);

  /*
   * Main-only section:
   * These accesses are deliberately unlocked, but are separated from worker
   * accesses by barriers.
   */
  double total = sum_abs;
  threshold = (float)(total / (double)N) * 1.5f;
  sum_abs = 0.0;

  /*
   * Release workers into Phase 2.
   */
  pthread_barrier_wait(&main_done);

  for (int t = 0; t < NUM_THREADS; t++) {
    pthread_join(threads[t], NULL);
  }

  printf("threshold=%f sum_above=%f sum_abs=%f\n",
         threshold,
         sum_above,
         sum_abs);

  pthread_barrier_destroy(&phase1_done);
  pthread_barrier_destroy(&main_done);

  pthread_mutex_destroy(&m1);
  pthread_mutex_destroy(&m2);

  free(x);
  free(y);

  return 0;
}