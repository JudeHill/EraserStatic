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
#include <stdlib.h>
#include <stdio.h>

#define NUM_THREADS 4
#define ITERS 1000

static pthread_t tids[NUM_THREADS];

static int account_a = 0;
static int account_b = 0;
static int total_updates = 0;
static int checksum = 0;

pthread_mutex_t mutex_1;
pthread_mutex_t mutex_2;
pthread_barrier_t phase_barrier;

static int do_work(int tid, int i) {
  int x = tid + 1;
  x = (x * 31 + i) % 97;
  x = (x * x + 17) % 101;
  return x;
}

void *worker(void *arg) {
  int tid = *(int *)arg;
  free(arg);

  int local_sum = 0;

  /*
   * Phase 1:
   *   account_a and checksum are protected by mutex_1.
   *   account_b and total_updates are protected by mutex_2.
   */
  for (int i = 0; i < ITERS; i++) {
    int delta = do_work(tid, i);
    local_sum += delta;

    pthread_mutex_lock(&mutex_1);
    account_a += delta;
    checksum ^= delta;
    pthread_mutex_unlock(&mutex_1);

    pthread_mutex_lock(&mutex_2);
    account_b -= delta;
    total_updates++;
    pthread_mutex_unlock(&mutex_2);
  }

  pthread_barrier_wait(&phase_barrier);

  /*
   * Phase 2:
   *   account_a and checksum are now protected by mutex_2.
   *   account_b and total_updates are now protected by mutex_1.
   */
  for (int i = 0; i < ITERS; i++) {
    int delta = do_work(tid, ITERS - i);
    local_sum -= delta;

    pthread_mutex_lock(&mutex_2);
    account_a -= delta;
    checksum ^= delta;
    pthread_mutex_unlock(&mutex_2);

    pthread_mutex_lock(&mutex_1);
    account_b += delta;
    total_updates++;
    pthread_mutex_unlock(&mutex_1);
  }

  pthread_mutex_lock(&mutex_1);
  checksum += local_sum; 
  pthread_mutex_unlock(&mutex_1);

  return NULL;
}

int main(void) {
  pthread_mutex_init(&mutex_1, NULL);
  pthread_mutex_init(&mutex_2, NULL);
  pthread_barrier_init(&phase_barrier, NULL, NUM_THREADS);

  for (int i = 0; i < NUM_THREADS; i++) {
    int *arg = malloc(sizeof(int));
    *arg = i;
    pthread_create(&tids[i], NULL, worker, arg);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(tids[i], NULL);
  }

  printf("account_a = %d\n", account_a);
  printf("account_b = %d\n", account_b);
  printf("total_updates = %d\n", total_updates);
  printf("checksum = %d\n", checksum);

  pthread_barrier_destroy(&phase_barrier);
  pthread_mutex_destroy(&mutex_1);
  pthread_mutex_destroy(&mutex_2);

  return 0;
}