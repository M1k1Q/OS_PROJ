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
      printf("Producer paused\n");
      continue;
    }

    int part_ID = rand() % 100;
    int part_t = rand() % 2;
    sleep(2);
    if (isShutDown) break;

    if (consumed_list.top == MAX_PARTS) {
      printf("consumer list full");
    }

    if (sem_trywait(&consumed_list.empty) != 0) {
      if (isShutDown) break;
      continue;
    }

    pthread_mutex_lock(&produced_list.mutex);
    if (produced_list.top >= MAX_PARTS) {
      pthread_mutex_unlock(&produced_list.mutex);
      sem_post(&produced_list.empty);
      continue;
    }
    Car_Part part = {.id = part_ID, .type = part_t};
    produced_list.stacc[produced_list.top++] = part;
    // printf("produced_list top = %d\n", produced_list.top);
    pthread_mutex_unlock(&produced_list.mutex);
    sem_post(&produced_list.full);

    if (prod_log && Prod_log_ready && Prod_log_written) {
      sem_wait(Prod_log_written);
      snprintf(prod_log->message, LOG_MSG_SIZE,
               "Producer %d: ID=%d,Type = %s\n ", dat->id, part.id,
               part.type == Interior ? "Interior" : "Exterior");
      sem_post(Prod_log_ready);
    }
  }

  return NULL;
}
