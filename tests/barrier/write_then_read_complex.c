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
#define N (1<<22)          // ~4 million elements
#define BINS 256
#define ITERS 10

static pthread_barrier_t barrier;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

/* Work data */
static uint8_t *data;

/* Shared aggregates (written under lock in phase 1, read unlocked in phase 2) */
static uint32_t hist[BINS];
static uint64_t global_sum = 0;

static inline uint8_t mix(uint32_t x) {
  // cheap deterministic mixing to simulate work
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return (uint8_t)x;
}

static void *worker(void *arg) {
  intptr_t tid = (intptr_t)arg;
  size_t chunk = (size_t)N / NUM_THREADS;
  size_t lo = (size_t)tid * chunk;
  size_t hi = (tid == NUM_THREADS - 1) ? (size_t)N : lo + chunk;

  for (int it = 0; it < ITERS; it++) {

    /* ---------------- Phase 1: build local histogram + local sum ---------------- */
    uint32_t local_hist[BINS] = {0};
    uint64_t local_sum = 0;

    for (size_t i = lo; i < hi; i++) {
      uint8_t v = mix((uint32_t)data[i] + (uint32_t)it);
      local_hist[v]++;
      local_sum += (uint64_t)v;
    }

    /* Merge into shared aggregates under a mutex (write phase) */
    pthread_mutex_lock(&m);
    for (int b = 0; b < BINS; b++) hist[b] += local_hist[b];
    global_sum += local_sum;
    pthread_mutex_unlock(&m);

    /* Barrier separates write phase from read phase */
    pthread_barrier_wait(&barrier);

    /* ---------------- Phase 2: unlocked snapshot + use it ---------------- */
    // Unlocked reads: lockset-only often flags (writes under m, reads without m)
    uint64_t sum_snapshot = global_sum;
    uint32_t hist0 = hist[0];
    uint32_t hist255 = hist[255];

    // Use snapshot to do more work so the reads matter
    // (e.g., compute a per-thread score / normalization)
    double mean = (double)sum_snapshot / (double)N;
    double score = (double)(hist0 + 1) / (double)(hist255 + 1) + mean * 0.001;

    // Touch data to prevent dead-code elimination and to make the phase do work
    for (size_t i = lo; i < hi; i += 4096) {
      data[i] = (uint8_t)((double)data[i] + score);
    }

    /* Optional second barrier to avoid overlap between iterations */
    pthread_barrier_wait(&barrier);

    /* Single thread resets aggregates for next iteration (under lock) */
    if (tid == 0) {
      pthread_mutex_lock(&m);
      for (int b = 0; b < BINS; b++) hist[b] = 0;
      global_sum = 0;
      pthread_mutex_unlock(&m);
    }

    pthread_barrier_wait(&barrier);
  }

  return NULL;
}

int main(void) {
  pthread_t threads[NUM_THREADS];

  data = (uint8_t*)malloc(N);
  if (!data) return 1;

  // Initialize work data
  for (size_t i = 0; i < N; i++) data[i] = (uint8_t)(i * 1315423911u);

  pthread_mutex_init(&m, NULL);
  pthread_barrier_init(&barrier, NULL, NUM_THREADS);

  for (intptr_t i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, worker, (void*)i);
  }
  for (int i = 0; i < NUM_THREADS; i++) pthread_join(threads[i], NULL);

  pthread_barrier_destroy(&barrier);
  pthread_mutex_destroy(&m);

  free(data);
  return 0;
}
