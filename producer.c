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
  part_Type my_type = dat->type;

  while (!isShutDown) {
    pthread_mutex_lock(&pauseMutex);
    int paused = isPaused;
    pthread_mutex_unlock(&pauseMutex);
    sleep(1);
    if (paused) {
      // printf("Producer %d paused\n", dat->id);
      sleep(1);
      continue;
    }

    int part_ID = rand() % 100;
    if (isShutDown) break;

    Part_Stack *target =
        (my_type == Interior) ? &produced_Int_list : &produced_Ext_list;

    if (sem_trywait(&target->empty) != 0) {
      if (isShutDown) break;
      continue;
    }

    pthread_mutex_lock(&target->mutex);
    if (target->top >= MAX_PARTS) {
      pthread_mutex_unlock(&target->mutex);
      sem_post(&target->empty);
      continue;
    }

    Car_Part part = {.id = part_ID, .type = my_type};
    target->stacc[target->top++] = part;
    pthread_mutex_unlock(&target->mutex);
    sem_post(&target->full);
    // printf("Producer %d pushing part ID=%d Type=%s\n", dat->id, part.id,
    //        part.type == Interior ? "Interior" : "Exterior");

    if (prod_log && Prod_log_ready && Prod_log_written) {
      sem_wait(Prod_log_written);
      snprintf(prod_log->message, LOG_MSG_SIZE, "Producer %d: ID=%d, Type=%s\n",
               dat->id, part.id, my_type == Interior ? "Interior" : "Exterior");
      sem_post(Prod_log_ready);
    }
  }
  // printf("Producer shutting down..\n");
  return NULL;
}
// final change
