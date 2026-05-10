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

#define T 4                 // threads per pool
#define N (1<<20)           // 1M elements
#define ITERS 20

static pthread_mutex_t L1 = PTHREAD_MUTEX_INITIALIZER; // pool1 lock
static pthread_mutex_t L2 = PTHREAD_MUTEX_INITIALIZER; // pool2 lock
static pthread_barrier_t phase_barrier;                // all workers

static float *A, *B;

// “Meaningful” shared stats (also protected by the pool lock that currently owns the buffer)
static double sumA = 0.0;
static double sumB = 0.0;

static inline uint32_t mix32(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}

/*
  Pool 1 semantics:
   - Phase 1: update A under L1, contribute to sumA under L1
   - Phase 2: update B under L1, contribute to sumB under L1
*/
static void *pool1_worker(void *arg) {
  intptr_t tid = (intptr_t)arg;

  size_t chunk = (size_t)N / T;
  size_t lo = (size_t)tid * chunk;
  size_t hi = (tid == T - 1) ? (size_t)N : lo + chunk;

  for (int it = 0; it < ITERS; it++) {
    // ----- Phase 1: pool1 owns A (lock L1) -----
    pthread_mutex_lock(&L1);
    double localA = 0.0;
    for (size_t i = lo; i < hi; i++) {
      // read-modify-write A[i] under L1
      uint32_t h = mix32((uint32_t)i + (uint32_t)(it * 131) + 17U);
      float v = A[i];
      v = v * (1.001f + (float)(h & 1023) * 0.000001f) - (float)(h & 7);
      A[i] = v;
      localA += (double)v;
    }
    sumA += localA; // shared scalar updated under L1
    pthread_mutex_unlock(&L1);

    pthread_barrier_wait(&phase_barrier);

    // ----- Phase 2: pool1 now owns B (still lock L1) -----
    pthread_mutex_lock(&L1);
    double localB = 0.0;
    for (size_t i = lo; i < hi; i++) {
      uint32_t h = mix32((uint32_t)i + (uint32_t)(it * 131) + 29U);
      float v = B[i];
      v = v * (0.999f + (float)(h & 1023) * 0.000001f) + (float)(h & 3);
      B[i] = v;
      localB += (double)v;
    }
    sumB += localB; // shared scalar updated under L1 (note: “wrong” lock for sumB in phase1)
    pthread_mutex_unlock(&L1);

    pthread_barrier_wait(&phase_barrier);
  }

  return NULL;
}

/*
  Pool 2 semantics:
   - Phase 1: update B under L2, contribute to sumB under L2
   - Phase 2: update A under L2, contribute to sumA under L2
*/
static void *pool2_worker(void *arg) {
  intptr_t tid = (intptr_t)arg;

  size_t chunk = (size_t)N / T;
  size_t lo = (size_t)tid * chunk;
  size_t hi = (tid == T - 1) ? (size_t)N : lo + chunk;

  for (int it = 0; it < ITERS; it++) {
    // ----- Phase 1: pool2 owns B (lock L2) -----
    pthread_mutex_lock(&L2);
    double localB = 0.0;
    for (size_t i = lo; i < hi; i++) {
      uint32_t h = mix32((uint32_t)i + (uint32_t)(it * 131) + 43U);
      float v = B[i];
      v = v * (1.002f + (float)(h & 1023) * 0.000001f) - (float)(h & 15);
      B[i] = v;
      localB += (double)v;
    }
    sumB += localB; // shared scalar updated under L2
    pthread_mutex_unlock(&L2);

    pthread_barrier_wait(&phase_barrier);

    // ----- Phase 2: pool2 now owns A (still lock L2) -----
    pthread_mutex_lock(&L2);
    double localA = 0.0;
    for (size_t i = lo; i < hi; i++) {
      uint32_t h = mix32((uint32_t)i + (uint32_t)(it * 131) + 59U);
      float v = A[i];
      v = v * (1.0005f + (float)(h & 1023) * 0.000001f) + (float)(h & 7);
      A[i] = v;
      localA += (double)v;
    }
    sumA += localA; // shared scalar updated under L2
    pthread_mutex_unlock(&L2);

    pthread_barrier_wait(&phase_barrier);
  }

  return NULL;
}

int main(void) {
  pthread_t p1[T], p2[T];

  A = (float*)malloc(sizeof(float) * N);
  B = (float*)malloc(sizeof(float) * N);
  if (!A || !B) return 1;

  for (size_t i = 0; i < N; i++) {
    A[i] = (float)(i & 1023) * 0.01f;
    B[i] = (float)((i * 7) & 1023) * 0.01f;
  }

  pthread_mutex_init(&L1, NULL);
  pthread_mutex_init(&L2, NULL);

  // All workers participate in each barrier (2 pools * T threads)
  pthread_barrier_init(&phase_barrier, NULL, 2 * T);

  for (intptr_t i = 0; i < T; i++) {
    pthread_create(&p1[i], NULL, pool1_worker, (void*)i);
    pthread_create(&p2[i], NULL, pool2_worker, (void*)i);
  }

  for (int i = 0; i < T; i++) {
    pthread_join(p1[i], NULL);
    pthread_join(p2[i], NULL);
  }

  pthread_barrier_destroy(&phase_barrier);
  pthread_mutex_destroy(&L1);
  pthread_mutex_destroy(&L2);

  printf("done A0=%f B0=%f sumA=%f sumB=%f\n", A[0], B[0], sumA, sumB);

  free(A);
  free(B);
  return 0;
}
