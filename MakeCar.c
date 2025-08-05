#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "factory.h"

#define LOG_READY "/CarMake_sem_log_ready"
#define LOG_WRITTEN "/CarMake_sem_log_written"
#define LOG_SHM "/CarMake_log_shm"

void *MakeCar() {
  while (!isShutDown) {
    pthread_mutex_lock(&pauseMutex);
    int paused = isPaused;
    pthread_mutex_unlock(&pauseMutex);
    sleep(2);
    if (paused) {
      // printf("MakeCar paused\n");
      sleep(1);
      continue;
    }

    if (isShutDown) break;
    if (sem_wait(&Ext_list.full) == -1) break;
    if (isShutDown) break;
    if (sem_wait(&Int_list.full) == -1) break;
    if (isShutDown) break;

    pthread_mutex_lock(&Ext_list.mutex);
    pthread_mutex_lock(&Int_list.mutex);

    if (Int_list.top <= 0 || Ext_list.top <= 0) {
      pthread_mutex_unlock(&Int_list.mutex);
      pthread_mutex_unlock(&Ext_list.mutex);
      sem_post(&Ext_list.empty);
      sem_post(&Int_list.empty);
      continue;
    }

    Car_Part Interior_part = Int_list.stacc[--Int_list.top];
    Car_Part Exterior_part = Ext_list.stacc[--Ext_list.top];

    pthread_mutex_unlock(&Int_list.mutex);
    pthread_mutex_unlock(&Ext_list.mutex);

    sem_post(&Int_list.empty);
    sem_post(&Ext_list.empty);

    // printf("Interior Part : %d | Exterior Part : %d \n", Interior_part.id,
    //        Exterior_part.id);

    if (Cm_log && Cm_log_ready && Cm_log_written) {
      sem_wait(Cm_log_written);
      sprintf(Cm_log->message,
              "Car produced : Interior ID = %d | Exterior ID = %d \n",
              Interior_part.id, Exterior_part.id);
      sem_post(Cm_log_ready);
    }
    Cars_produced++;
  }

  // printf("MakeCar shutting down...\n");
  return NULL;
}
