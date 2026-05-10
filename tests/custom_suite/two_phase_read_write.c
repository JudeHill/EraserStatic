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
static pthread_mutex_t Lw1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t Lw2 = PTHREAD_MUTEX_INITIALIZER;

static int BUF1 = 0;
static int BUF2 = 0;
static int RACEY = 0;

static void *worker(void *arg) {
  (void)arg;

  // Phase 1: write BUF1 under Lw1, read BUF2 without lock (read-only in this phase)
  pthread_mutex_lock(&Lw1);
  BUF1 += 1;
  RACEY += 1;
  pthread_mutex_unlock(&Lw1);

  int r1 = BUF2; (void)r1;
  int r2 = RACEY; (void)r2;

  pthread_barrier_wait(&B);

  // Phase 2: write BUF2 under Lw2, read BUF1 without lock (read-only in this phase)
  pthread_mutex_lock(&Lw2);
  BUF2 += 1;
  pthread_mutex_unlock(&Lw2);

  int r2 = BUF1; (void)r2;

  return NULL;
}

int main(int argc, char **argv) {
  int n = (argc > 1) ? atoi(argv[1]) : 4;
  pthread_t *t = malloc(sizeof(*t) * n);

  pthread_barrier_init(&B, NULL, (unsigned)n);
  for (int i = 0; i < n; i++) pthread_create(&t[i], NULL, worker, NULL);
  for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
  pthread_barrier_destroy(&B);

  printf("BUF1=%d BUF2=%d\n", BUF1, BUF2);
  free(t);
}
