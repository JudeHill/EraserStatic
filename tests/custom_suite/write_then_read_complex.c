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
#define N (1<<22)
#define BINS 256

static pthread_barrier_t barrier;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

static uint8_t *data;

static uint32_t hist[BINS];
static uint64_t global_sum = 0;

static inline uint8_t mix(uint32_t x) {
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

  /*
   * Phase 1:
   * Each worker builds a local histogram and merges it into the
   * shared aggregates under the mutex.
   */
  uint32_t local_hist[BINS] = {0};
  uint64_t local_sum = 0;

  for (size_t i = lo; i < hi; i++) {
    uint8_t v = mix((uint32_t)data[i]);
    local_hist[v]++;
    local_sum += (uint64_t)v;
  }

  pthread_mutex_lock(&m);
  for (int b = 0; b < BINS; b++) {
    hist[b] += local_hist[b];
  }
  global_sum += local_sum;
  pthread_mutex_unlock(&m);

  pthread_barrier_wait(&barrier);

  /*
   * Phase 2:
   * Unlocked reads from hist/global_sum.
   */
  uint64_t sum_snapshot = global_sum;
  uint32_t hist0 = hist[0];
  uint32_t hist255 = hist[255];

  double mean = (double)sum_snapshot / (double)N;
  double score = (double)(hist0 + 1) / (double)(hist255 + 1) + mean * 0.001;

  for (size_t i = lo; i < hi; i += 4096) {
    data[i] = (uint8_t)((double)data[i] + score);
  }

  return NULL;
}

int main(void) {
  pthread_t threads[NUM_THREADS];

  data = (uint8_t*)malloc(N);
  if (!data) return 1;

  for (size_t i = 0; i < N; i++) {
    data[i] = (uint8_t)(i * 1315423911u);
  }

  pthread_mutex_init(&m, NULL);
  pthread_barrier_init(&barrier, NULL, NUM_THREADS);

  for (intptr_t i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, worker, (void*)i);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  printf("global_sum=%llu hist[0]=%u hist[255]=%u\n",
         (unsigned long long)global_sum,
         hist[0],
         hist[255]);

  pthread_barrier_destroy(&barrier);
  pthread_mutex_destroy(&m);

  free(data);
  return 0;
}