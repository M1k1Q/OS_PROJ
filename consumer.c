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

    if (part.type == Exterior) {
      pthread_mutex_lock(&Ext_list.mutex);
      if (Ext_list.top < MAX_PARTS) {
        Ext_list.stacc[Ext_list.top++] = part;
      } else {
        printf("Exterior list has been filled \n");
      }
      pthread_mutex_unlock(&Ext_list.mutex);
      sem_post(&Ext_list.full);
    } else if (part.type == Interior) {
      pthread_mutex_lock(&Int_list.mutex);
      if (Ext_list.top < MAX_PARTS) {
        Int_list.stacc[Int_list.top++] = part;
      } else {
        printf("Interior list has been filled \n");
      }
      pthread_mutex_unlock(&Int_list.mutex);
      sem_post(&Int_list.full);
    }

    if (cons_log && Cons_log_ready && Cons_log_written) {
      sem_wait(Cons_log_written);
      snprintf(cons_log->message, LOG_MSG_SIZE,
               "Consumer: Read ID=%d, Type=%s\n", part.id,
               part.type == Interior ? "Interior" : "Exterior");
      sem_post(Cons_log_ready);
    }

    printf("Exterior list stack top : %d | Interior list stack top : %d\n",
           Ext_list.top, Int_list.top);
  }

  return NULL;
}
