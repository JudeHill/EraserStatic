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

static int inventory_count = 1000;
static int reservations = 0;
static int shipments = 0;
static int audit_total = 0;

pthread_mutex_t mutex_1;
pthread_mutex_t mutex_2;
pthread_barrier_t handoff_barrier;

static int work_amount(int tid, int i) {
  return ((tid + 1) * (i + 3)) % 7;
}

void *worker(void *arg) {
  int tid = *(int *)arg;
  free(arg);

  int local_reserved = 0;
  int local_shipped = 0;

  /*
   * Phase 1:
   * inventory_count is protected by mutex_1.
   * Threads reserve items from inventory.
   */
  for (int i = 0; i < ITERS; i++) {
    int amount = work_amount(tid, i);

    pthread_mutex_lock(&mutex_1);
    if (inventory_count >= amount) {
      inventory_count -= amount;
      reservations += amount;
      local_reserved += amount;
    }
    pthread_mutex_unlock(&mutex_1);
  }

  pthread_barrier_wait(&handoff_barrier);

  /*
   * Phase 2:
   * inventory_count is now handed off to mutex_2.
   */
  for (int i = 0; i < ITERS; i++) {
    int amount = work_amount(tid, ITERS - i);

    pthread_mutex_lock(&mutex_2);
    inventory_count += amount / 2;
    shipments += amount;
    local_shipped += amount;
    pthread_mutex_unlock(&mutex_2);
  }

  pthread_mutex_lock(&mutex_1);
  audit_total += local_reserved - local_shipped;
  pthread_mutex_unlock(&mutex_1);

  return NULL;
}

int main(void) {
  pthread_mutex_init(&mutex_1, NULL);
  pthread_mutex_init(&mutex_2, NULL);
  pthread_barrier_init(&handoff_barrier, NULL, NUM_THREADS);

  for (int i = 0; i < NUM_THREADS; i++) {
    int *arg = malloc(sizeof(int));
    *arg = i;
    pthread_create(&tids[i], NULL, worker, arg);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(tids[i], NULL);
  }

  printf("inventory_count = %d\n", inventory_count);
  printf("reservations = %d\n", reservations);
  printf("shipments = %d\n", shipments);
  printf("audit_total = %d\n", audit_total);

  pthread_barrier_destroy(&handoff_barrier);
  pthread_mutex_destroy(&mutex_1);
  pthread_mutex_destroy(&mutex_2);

  return 0;
}