#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "factory.h"

void *Prod(void *arg) {
  Thread_data *dat = (Thread_data *)arg;

  while (!isShutDown) {
    pthread_mutex_lock(&pauseMutex);
    int paused = isPaused;
    pthread_mutex_unlock(&pauseMutex);

    if (paused == 1) {
      // sleep(1);
      continue;
    }

    int part_ID = rand() % 100;
    int part_t = rand() % 2;
    sleep(1);
    if (isShutDown) break;

    if (sem_trywait(&parts.empty) != 0) {
      if (isShutDown) break;
      continue;
    }

    pthread_mutex_lock(&parts.mutex);
    if (parts.top >= MAX_PARTS) {
      pthread_mutex_unlock(&parts.mutex);
      sem_post(&parts.empty);
      continue;
    }
    Car_Part part = {.id = part_ID, .type = part_t};
    parts.stacc[parts.top++] = part;
    printf("parts top = %d\n", parts.top);
    pthread_mutex_unlock(&parts.mutex);
    sem_post(&parts.full);

    if (prod_log && Prod_log_ready && Prod_log_written) {
      sem_wait(Prod_log_written);
      snprintf(prod_log->message, LOG_MSG_SIZE,
               "Producer %d: ID=%d,Type = % s\n ", dat->id, part.id,
               part.type == Interior ? "Interior" : "Exterior");
      sem_post(Prod_log_ready);
    }
  }

  return NULL;
}