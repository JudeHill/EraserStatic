/* 
 * Project: EraserStatic
 * (https://github.com/JudeHill/EraserStatic)
 *
 * Copyright (C) 2025-2026 Jude Hill <jude-stephen-hill@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NUM_THREADS 4
#define N_NODES (1<<18)      // ~262k
#define N_EDGES (1<<20)      // ~1M
#define ITERS 10

/* Stage-1 shared structure: a "frontier" (dynamic worklist) */
static int *frontier;
static int frontier_size = 0;
static pthread_mutex_t mutex_frontier = PTHREAD_MUTEX_INITIALIZER;

/* Stage-2 shared structure: global stats */
static uint64_t total_hits = 0;
static uint64_t total_cost = 0;
static pthread_mutex_t mutex_stats = PTHREAD_MUTEX_INITIALIZER;

/* Barrier between phases */
static pthread_barrier_t barrier;

/* Synthetic "graph" edges: src[i] -> dst[i] */
static uint32_t *src, *dst;

/* Node labels we update across iters */
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

    /* -------- Phase 1: build frontier (lock frontier mutex) -------- */
    for (size_t i = lo; i < hi; i++) {
      uint32_t u = src[i];
      uint32_t v = dst[i];

      // Synthetic "discover" condition: depends on active[u] and iteration
      if (active[u] && ((mix32(u + v + (uint32_t)it) & 0xFF) == 0)) {
        pthread_mutex_lock(&mutex_frontier);
        frontier[frontier_size++] = (int)v;
        pthread_mutex_unlock(&mutex_frontier);
      }
    }

    pthread_barrier_wait(&barrier);

    /* -------- Phase 2: process frontier and update stats (different lock) --------
       Each worker walks every NUM_THREADS-th element to avoid needing tid==... branches
       for correctness (still just work distribution). */
    uint64_t local_hits = 0;
    uint64_t local_cost = 0;

    int fsz = frontier_size; // unlocked read is safe after barrier under your model
    for (int j = (int)tid; j < fsz; j += NUM_THREADS) {
      int node = frontier[j];

      // Do some compute, update "active" (writes are racy unless you protect them;
      // so we only read active here to keep the benchmark focused)
      uint32_t h = mix32((uint32_t)node + (uint32_t)it);
      local_cost += (h & 1023);
      local_hits += (h & 1);
    }

    pthread_mutex_lock(&mutex_stats);
    total_hits += local_hits;
    total_cost += local_cost;
    pthread_mutex_unlock(&mutex_stats);

    pthread_barrier_wait(&barrier);

    /* Reset frontier for next iteration (single-thread reset guarded by frontier lock) */
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

  // Initialize synthetic edges and active set
  for (size_t i = 0; i < N_EDGES; i++) {
    src[i] = (uint32_t)(mix32((uint32_t)i) % N_NODES);
    dst[i] = (uint32_t)(mix32((uint32_t)i + 12345) % N_NODES);
  }
  for (size_t i = 0; i < N_NODES; i++) active[i] = (uint8_t)((i & 7) == 0);

  pthread_barrier_init(&barrier, NULL, NUM_THREADS);

  for (intptr_t t = 0; t < NUM_THREADS; t++) {
    pthread_create(&threads[t], NULL, worker, (void*)t);
  }
  for (int t = 0; t < NUM_THREADS; t++) pthread_join(threads[t], NULL);

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
