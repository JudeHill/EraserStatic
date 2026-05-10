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
static int tids[NUM_THREADS];
static int output[NUM_THREADS];
static int count = 0;
pthread_mutex_t mutex_1;
pthread_barrier_t b1;

void *print_tid(void *arg) {
  int tid = *((int *)arg);
  output[tid] = tid;
  // pthread_mutex_lock(&mutex_1);
  count++;
  // pthread_mutex_unlock(&mutex_1);
}

int main(void) {
  pthread_t threads[10];
  pthread_mutex_init(&mutex_1, NULL);
  for (int i = 0; i < 10;i++) {
    int Error = pthread_create(&threads[i], NULL, (void *(*)(void *))(print_tid), NULL);
  }

  for (int i = 0; i < 5; i++) {
    pthread_join(threads[i], &tids[i]);
  }
  for (int i = 5; i < 10; i++) {
    pthread_join(threads[i], &tids[i]);
  }
  return 0;
}