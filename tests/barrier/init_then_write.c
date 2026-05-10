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

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_barrier_t B;

static int CONFIG = 0;
static int DATA = 0;

static void *worker(void *arg) {
  (void)arg;

  // Phase 1: all threads cooperatively "initialize"
  // (e.g. last-writer-wins, but still race-free if you ensure single-writer;
  // here we do deterministic init via barrier staging)
  CONFIG = 42; // naive detector flags writes

  pthread_barrier_wait(&B);

  // Phase 2: CONFIG treated as immutable, read-only
  if (CONFIG == 42) DATA++;

  return NULL;
}

int main(int argc, char **argv) {
  int n = (argc > 1) ? atoi(argv[1]) : 4;
  pthread_t *t = malloc(sizeof(*t) * n);

  pthread_barrier_init(&B, NULL, (unsigned)n);
  for (int i = 0; i < n; i++) pthread_create(&t[i], NULL, worker, NULL);
  for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
  pthread_barrier_destroy(&B);

  printf("CONFIG=%d DATA=%d\n", CONFIG, DATA);
  free(t);
}
