#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NUM_THREADS 4
#define N_NODES (1<<18)
#define N_EDGES (1<<20)
#define ITERS 10

static int *frontier;
static int frontier_size = 0;

/* Phase 1 owns frontier under this lock */
static pthread_mutex_t mutex_frontier = PTHREAD_MUTEX_INITIALIZER;

/* Phase 2 owns frontier/statistics under this lock */
static pthread_mutex_t mutex_stats = PTHREAD_MUTEX_INITIALIZER;

static uint64_t total_hits = 0;
static uint64_t total_cost = 0;

static pthread_barrier_t barrier;

static uint32_t *src, *dst;
static uint8_t *active;

static inline uint32_t mix32(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}

static void *worker(void *arg) {
  intptr_t tid = (intptr_t)arg;

  size_t chunk = (size_t)N_EDGES / NUM_THREADS;
  size_t lo = (size_t)tid * chunk;
  size_t hi = (tid == NUM_THREADS - 1) ? (size_t)N_EDGES : lo + chunk;

  for (int it = 0; it < ITERS; it++) {

    /*
     * Phase 1:
     * frontier and frontier_size are protected by mutex_frontier.
     */
    for (size_t i = lo; i < hi; i++) {
      uint32_t u = src[i];
      uint32_t v = dst[i];

      if (active[u] && ((mix32(u + v + (uint32_t)it) & 0xFF) == 0)) {
        pthread_mutex_lock(&mutex_frontier);
        frontier[frontier_size++] = (int)v;
        pthread_mutex_unlock(&mutex_frontier);
      }
    }

    /*
     * Handoff point:
     * after this barrier, no thread will access frontier using mutex_frontier
     * until the next iteration reset.
     */
    pthread_barrier_wait(&barrier);

    /*
     * Phase 2:
     * frontier and frontier_size are now protected by mutex_stats.
     *
     * This is the handoff pattern:
     *   Phase 1: frontier protected by mutex_frontier
     *   Phase 2: frontier protected by mutex_stats
     * with the barrier separating the two phases.
     */
    uint64_t local_hits = 0;
    uint64_t local_cost = 0;

    pthread_mutex_lock(&mutex_stats);

    int fsz = frontier_size;

    for (int j = (int)tid; j < fsz; j += NUM_THREADS) {
      int node = frontier[j];

      uint32_t h = mix32((uint32_t)node + (uint32_t)it);
      local_cost += (h & 1023);
      local_hits += (h & 1);
    }

    total_hits += local_hits;
    total_cost += local_cost;

    pthread_mutex_unlock(&mutex_stats);

    pthread_barrier_wait(&barrier);

    /*
     * Reset for next iteration.
     * This returns ownership of frontier/frontier_size to mutex_frontier.
     */
    if (tid == 0) {
      pthread_mutex_lock(&mutex_frontier);
      frontier_size = 0;
      pthread_mutex_unlock(&mutex_frontier);
    }

    pthread_barrier_wait(&barrier);
  }

  return NULL;
}

int main(void) {
  pthread_t threads[NUM_THREADS];

  frontier = (int*)malloc(sizeof(int) * N_EDGES);
  src = (uint32_t*)malloc(sizeof(uint32_t) * N_EDGES);
  dst = (uint32_t*)malloc(sizeof(uint32_t) * N_EDGES);
  active = (uint8_t*)malloc(sizeof(uint8_t) * N_NODES);

  if (!frontier || !src || !dst || !active) return 1;

  for (size_t i = 0; i < N_EDGES; i++) {
    src[i] = (uint32_t)(mix32((uint32_t)i) % N_NODES);
    dst[i] = (uint32_t)(mix32((uint32_t)i + 12345) % N_NODES);
  }

  for (size_t i = 0; i < N_NODES; i++) {
    active[i] = (uint8_t)((i & 7) == 0);
  }

  pthread_barrier_init(&barrier, NULL, NUM_THREADS);

  for (intptr_t t = 0; t < NUM_THREADS; t++) {
    pthread_create(&threads[t], NULL, worker, (void*)t);
  }

  for (int t = 0; t < NUM_THREADS; t++) {
    pthread_join(threads[t], NULL);
  }

  pthread_barrier_destroy(&barrier);

  printf("total_hits=%llu total_cost=%llu frontier_size=%d\n",
         (unsigned long long)total_hits,
         (unsigned long long)total_cost,
         frontier_size);

  free(frontier);
  free(src);
  free(dst);
  free(active);

  return 0;
}