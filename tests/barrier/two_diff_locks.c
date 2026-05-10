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

#define NUM_THREADS 4
static pthread_t tids[NUM_THREADS];
static int count = 0;
pthread_mutex_t mutex_1;
pthread_mutex_t mutex_2;
pthread_barrier_t barrier;

void *worker(void *arg) {
  pthread_mutex_lock(&mutex_1);
  count++;
  pthread_mutex_unlock(&mutex_1);
  pthread_barrier_wait(&barrier);
  pthread_mutex_lock(&mutex_2);
  count++;
  pthread_mutex_unlock(&mutex_2);
  pthread_barrier_wait(&barrier);
  pthread_mutex_lock(&mutex_1);
  count++;
  pthread_mutex_lock(&mutex_1);
}

int main(void) {
  pthread_t threads[NUM_THREADS];
  count = 10;
  pthread_mutex_init(&mutex_1, NULL);
  pthread_barrier_init(&barrier, NULL, NUM_THREADS);
  for (int i = 0; i < NUM_THREADS;i++) {
    int *arg = malloc(sizeof(int));
    *arg = i;
    int Error = pthread_create(&threads[i], NULL, (void *(*)(void *))(worker), NULL);
    // pthread_create(&threads[i], NULL, (void * (*))(print_tid), NULL);
  }
  

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
    count++;
  }
  count = 6;
  pthread_barrier_destroy(&barrier);

  return 0;
}