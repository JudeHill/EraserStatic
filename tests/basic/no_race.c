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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Number of threads
#define NUM_THREADS 1000

struct shared_data {
    int value;
    pthread_mutex_t lock;
};

int global = 0;
pthread_mutex_t global_lock;
// Function executed by each thread
void* print_message(void* arg) {
    pthread_mutex_lock(&global_lock);
    global++;
    printf("Incremented global to %d\n", global);
    pthread_mutex_unlock(&global_lock);

    
}


int main() {
    pthread_t threads[NUM_THREADS];
    int rc;
    long t;
    struct shared_data data = { .value = 0};
    pthread_mutex_init(&data.lock, NULL);
    pthread_mutex_init(&global_lock, NULL);
    global++;

    for (t = 0; t < NUM_THREADS; t++) {
        printf("Creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, print_message, (void*) &data);
        if (rc) {
            fprintf(stderr, "Error: unable to create thread, %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to finish
    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }
    pthread_mutex_destroy(&data.lock);
    printf("All threads completed.\n");
    pthread_exit(NULL);
}
