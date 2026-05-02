#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NUM_THREADS 4
#define N (1<<20)          // ~1 million elements
#define ITERS 20

static float *x;           // shared array
static float *y;           // shared array

static pthread_barrier_t phase1_done;   // workers + main
static pthread_barrier_t phase2_done;   // workers + main

static pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

/* Shared scalars updated in different phases */
static double sum_abs = 0.0;        // produced in phase 1 (mutex m1)
static float  threshold = 0.0f;     // produced by main between phases
static double sum_above = 0.0;      // produced in phase 2 (mutex m2)

static inline float f_abs(float v) { return v < 0 ? -v : v; }

/* A tiny compute-heavy-ish transform to make the loop do "work" */
static inline float transform(float v, int it) {
  // deterministic arithmetic, no libm dependency
  float t = v * (1.01f + 0.0001f * it);
  t = t - (t * t) * 0.000001f;
  return t;
}

static void *worker(void *arg) {
  intptr_t tid = (intptr_t)arg;
  size_t chunk = (size_t)N / NUM_THREADS;
  size_t lo = (size_t)tid * chunk;
  size_t hi = (tid == NUM_THREADS - 1) ? (size_t)N : lo + chunk;

  for (int it = 0; it < ITERS; it++) {

    /* ---------------- Phase 1: compute y = transform(x), accumulate sum_abs ---------------- */
    double local_sum = 0.0;
    for (size_t i = lo; i < hi; i++) {
      float v = transform(x[i], it);
      y[i] = v;
      local_sum += (double)f_abs(v);
    }

    /* Publish local contribution under m1 */
    pthread_mutex_lock(&m1);
    sum_abs += local_sum;
    pthread_mutex_unlock(&m1);

    /* All workers + main rendezvous: phase 1 complete */
    pthread_barrier_wait(&phase1_done);

    /* ---------------- Between phases: main sets threshold ----------------
       Workers don't touch threshold until after phase2_done barrier below. */

    /* Wait for main to publish threshold */
    pthread_barrier_wait(&phase2_done);

    /* ---------------- Phase 2: count/accumulate elements above threshold, update x ---------------- */
    double local_above = 0.0;
    float thr = threshold;   // unlocked read: safe due to barrier HB

    for (size_t i = lo; i < hi; i++) {
      float v = y[i];
      if (f_abs(v) > thr) local_above += (double)f_abs(v);

      // update x for next iteration
      x[i] = v * 0.99f;
    }

    pthread_mutex_lock(&m2);
    sum_above += local_above;
    pthread_mutex_unlock(&m2);

    /* End of iteration: main can read sum_above after join/barrier if you add one more barrier.
       For simplicity, we reset in main before next iteration at phase1 barrier. */
  }

  return NULL;
}

int main(void) {
  pthread_t threads[NUM_THREADS];

  x = (float*)malloc(sizeof(float) * N);
  y = (float*)malloc(sizeof(float) * N);
  if (!x || !y) return 1;

  // init data
  for (size_t i = 0; i < N; i++) x[i] = (float)(i % 1000) * 0.001f;

  pthread_barrier_init(&phase1_done, NULL, NUM_THREADS + 1);
  pthread_barrier_init(&phase2_done, NULL, NUM_THREADS + 1);

  pthread_mutex_init(&m1, NULL);
  pthread_mutex_init(&m2, NULL);

  for (intptr_t t = 0; t < NUM_THREADS; t++) {
    pthread_create(&threads[t], NULL, worker, (void*)t);
  }

  for (int it = 0; it < ITERS; it++) {

    /* Wait until all workers finished phase 1 and published sum_abs */
    pthread_barrier_wait(&phase1_done);

    /* Main thread "global decision": compute threshold from sum_abs */
    double total = 0.0;

    // unlocked read would be a lockset FP candidate (sum_abs written under m1)
    // but main is synchronized by barrier with all workers.
    total = sum_abs;

    // choose a threshold based on mean abs value
    threshold = (float)(total / (double)N) * 1.5f;

    // reset for next iteration (phase 1 will repopulate)
    sum_abs = 0.0;

    // also reset phase2 accumulator so iteration is self-contained
    sum_above = 0.0;

    /* Publish threshold (and resets) to workers */
    pthread_barrier_wait(&phase2_done);

    // (Optional) could wait for a third barrier here if you want main to read sum_above safely each iter
  }

  for (int t = 0; t < NUM_THREADS; t++) pthread_join(threads[t], NULL);

  pthread_barrier_destroy(&phase1_done);
  pthread_barrier_destroy(&phase2_done);

  pthread_mutex_destroy(&m1);
  pthread_mutex_destroy(&m2);

  printf("done, threshold=%f\n", threshold);

  free(x);
  free(y);
  return 0;
}
