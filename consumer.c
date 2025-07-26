#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "factory.h"

void *Cons() {
  // Thread_data *dat = (Thread_data *)arg;

  while (!isShutDown) {
    sleep(2);
    if (isShutDown) break;
    if (sem_trywait(&produced_list.full) != 0) {
      if (isShutDown) break;
      // printf("[Cons] No produced_list in stack . Waiting...\n");
      continue;
    }

    pthread_mutex_lock(&produced_list.mutex);
    if (produced_list.top <= 0) {
      pthread_mutex_unlock(&produced_list.mutex);
      sem_post(&produced_list.full);
      continue;
    }

    Car_Part part = produced_list.stacc[--produced_list.top];
    pthread_mutex_unlock(&produced_list.mutex);
    sem_post(&produced_list.empty);

    pthread_mutex_lock(&consumed_list.mutex);
    if (consumed_list.top < MAX_PARTS) {
      consumed_list.stacc[consumed_list.top++] = part;
    } else {
      printf("consumer list has been filled \n");
    }
    pthread_mutex_unlock(&consumed_list.mutex);
    sem_post(&consumed_list.full);
    if (cons_log && Cons_log_ready && Cons_log_written) {
      sem_wait(Cons_log_written);
      snprintf(cons_log->message, LOG_MSG_SIZE,
               "Consumer: Read ID=%d, Type=%s\n", part.id,
               part.type == Interior ? "Interior" : "Exterior");
      sem_post(Cons_log_ready);
    }

    printf("Consumer stack top : %d\n", consumed_list.top);
  }

  return NULL;
}
