#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "factory.h"

void *Prod(void *arg) {
  Thread_data *dat = (Thread_data *)arg;

  while (!isShutDown) {
    pthread_mutex_lock(&pauseMutex);
    int paused = isPaused;
    pthread_mutex_unlock(&pauseMutex);

    if (paused) {
      printf("Producer %d paused\n", dat->id);
      sleep(1);
      continue;
    }

    int part_ID = rand() % 100;
    part_Type part_t = rand() % 2;
    sleep(2);
    if (isShutDown) break;

    if (part_t == Interior) {
      pthread_mutex_lock(&Int_list.mutex);
      if (Int_list.top >= MAX_PARTS) {
        pthread_mutex_unlock(&Int_list.mutex);
        printf("Producer %d waiting: Interior list full\n", dat->id);
        continue;
      }
      pthread_mutex_unlock(&Int_list.mutex);
    } else {
      pthread_mutex_lock(&Ext_list.mutex);
      if (Ext_list.top >= MAX_PARTS) {
        pthread_mutex_unlock(&Ext_list.mutex);
        printf("Producer %d waiting: Exterior list full\n", dat->id);
        continue;
      }
      pthread_mutex_unlock(&Ext_list.mutex);
    }

    if (sem_trywait(&produced_list.empty) != 0) {
      if (isShutDown) break;
      continue;
    }

    pthread_mutex_lock(&produced_list.mutex);
    if (produced_list.top >= MAX_PARTS) {
      pthread_mutex_unlock(&produced_list.mutex);
      sem_post(&produced_list.empty);
      continue;
    }

    // === PRODUCE PART ===
    Car_Part part = {.id = part_ID, .type = part_t};
    produced_list.stacc[produced_list.top++] = part;
    pthread_mutex_unlock(&produced_list.mutex);
    sem_post(&produced_list.full);

    // === LOGGING ===
    if (prod_log && Prod_log_ready && Prod_log_written) {
      sem_wait(Prod_log_written);
      snprintf(prod_log->message, LOG_MSG_SIZE, "Producer %d: ID=%d, Type=%s\n",
               dat->id, part.id,
               part.type == Interior ? "Interior" : "Exterior");
      sem_post(Prod_log_ready);
    }
  }

  return NULL;
}
