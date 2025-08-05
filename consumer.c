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

void *Cons(void *arg) {
  Thread_data *dat = (Thread_data *)arg;
  part_Type target_type = dat->type;

  Part_Stack *source_stack =
      (target_type == Interior) ? &produced_Int_list : &produced_Ext_list;
  Part_Stack *target_stack = (target_type == Interior) ? &Int_list : &Ext_list;

  while (!isShutDown) {
    // printf("cons running\n");
    pthread_mutex_lock(&pauseMutex);
    int paused = isPaused;
    pthread_mutex_unlock(&pauseMutex);

    sleep(2);
    if (paused) {
      // printf("Consumer %d paused\n", dat->id);
      sleep(1);
      continue;
    }

    if (isShutDown) break;

    if (sem_trywait(&source_stack->full) != 0) {
      if (isShutDown) break;
      continue;
    }

    pthread_mutex_lock(&source_stack->mutex);
    if (source_stack->top <= 0) {
      pthread_mutex_unlock(&source_stack->mutex);
      sem_post(&source_stack->full);
      continue;
    }

    Car_Part part = source_stack->stacc[--source_stack->top];
    pthread_mutex_unlock(&source_stack->mutex);
    sem_post(&source_stack->empty);

    pthread_mutex_lock(&target_stack->mutex);
    if (target_stack->top < MAX_PARTS) {
      target_stack->stacc[target_stack->top++] = part;
    }
    pthread_mutex_unlock(&target_stack->mutex);
    sem_post(&target_stack->full);

    // printf("Consumer %d pulling part ID=%d Type=%s\n", dat->id, part.id,
    //        part.type == Interior ? "Interior" : "Exterior");

    if (cons_log && Cons_log_ready && Cons_log_written) {
      sem_wait(Cons_log_written);
      snprintf(cons_log->message, LOG_MSG_SIZE,
               "Consumer %d: Read ID=%d, Type=%s\n", dat->id, part.id,
               part.type == Interior ? "Interior" : "Exterior");
      sem_post(Cons_log_ready);
    }
  }
  // printf("Consumer shutting down...\n");
  return NULL;
}
