#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "factory.h"

void *Cons(void *arg) {
  // Thread_data *dat = (Thread_data *)arg;

  while (!isShutDown) {
    sleep(1);
    if (isShutDown) break;

    if (sem_trywait(&parts.full) != 0) {
      if (isShutDown) break;
      printf("[Cons] No parts in stack . Waiting...\n");
      continue;
    }

    pthread_mutex_lock(&parts.mutex);
    if (parts.top <= 0) {
      pthread_mutex_unlock(&parts.mutex);
      sem_post(&parts.full);
      continue;
    }

    Car_Part part = parts.stacc[--parts.top];
    pthread_mutex_unlock(&parts.mutex);
    sem_post(&parts.empty);

    if (cons_log && Cons_log_ready && Cons_log_written) {
      sem_wait(Cons_log_written);
      snprintf(cons_log->message, LOG_MSG_SIZE,
               "Consumer: Read ID=%d, Type=%s\n", part.id,
               part.type == Interior ? "Interior" : "Exterior");
      sem_post(Cons_log_ready);
    }
  }

  return NULL;
}