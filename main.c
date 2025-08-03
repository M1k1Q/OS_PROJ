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

int numthreads = 0;
int numparts = 0;
volatile __sig_atomic_t isPaused = 0;
volatile __sig_atomic_t isShutDown = 0;
pthread_mutex_t pauseMutex;

// Part_Stack produced_list;
Part_Stack Ext_list;
Part_Stack Int_list;
Part_Stack produced_Int_list;
Part_Stack produced_Ext_list;

SharedLog *prod_log = NULL;
SharedLog *cons_log = NULL;
SharedLog *Cm_log = NULL;

sem_t *Prod_log_ready = NULL;
sem_t *Prod_log_written = NULL;
sem_t *Cons_log_ready = NULL;
sem_t *Cons_log_written = NULL;
sem_t *Cm_log_ready = NULL;
sem_t *Cm_log_written = NULL;

void *Manager() {
  char cmd;
  while (1) {
    printf("Enter command (p to pause , r to resume , q to quit): \n");
    cmd = getchar();
    getchar();

    pthread_mutex_lock(&pauseMutex);
    if (cmd == 'p') {
      isPaused = 1;
      printf("Paused\n");
    } else if (cmd == 'r') {
      isPaused = 0;
      printf("Resumed\n");
    } else if (cmd == 'q') {
      isShutDown = 1;
      printf("Shutting down\n");
      sem_post(&Ext_list.full);
      sem_post(&Int_list.full);
      sem_post(&produced_Int_list.full);
      sem_post(&produced_Ext_list.full);
      sem_post(&Ext_list.empty);
      sem_post(&Int_list.empty);
      pthread_mutex_unlock(&pauseMutex);
      break;
    }
    pthread_mutex_unlock(&pauseMutex);
  }
  return NULL;
}

void InitFactory() {
  produced_Ext_list.top = 0;
  produced_Int_list.top = 0;
  Ext_list.top = 0;
  Int_list.top = 0;
  pthread_mutex_init(&produced_Ext_list.mutex, NULL);
  sem_init(&produced_Ext_list.empty, 0, MAX_PARTS);
  sem_init(&produced_Ext_list.full, 0, 0);
  pthread_mutex_init(&produced_Int_list.mutex, NULL);
  sem_init(&produced_Int_list.empty, 0, MAX_PARTS);
  sem_init(&produced_Int_list.full, 0, 0);
  pthread_mutex_init(&Ext_list.mutex, NULL);
  sem_init(&Ext_list.empty, 0, MAX_PARTS);
  sem_init(&Ext_list.full, 0, 0);
  pthread_mutex_init(&Int_list.mutex, NULL);
  sem_init(&Int_list.empty, 0, MAX_PARTS);
  sem_init(&Int_list.full, 0, 0);
  srand(time(NULL));

  pthread_mutex_init(&pauseMutex, NULL);

  int prod_fd = shm_open(PROD_SHM, O_CREAT | O_RDWR, 0666);
  if (prod_fd == -1) {
    perror("Producer : SHM_OPEN FAILED...\n");
    exit(1);
  }

  if (ftruncate(prod_fd, sizeof(SharedLog)) == -1) {
    perror("Producer : : FTRUNCATE FAILED...\n");
    exit(1);
  }

  prod_log = mmap(NULL, sizeof(SharedLog), PROT_READ | PROT_WRITE, MAP_SHARED,
                  prod_fd, 0);
  if (prod_log == MAP_FAILED) {
    perror("Producer : MMAP FAILED...\n");
    exit(1);
  }

  Prod_log_ready = sem_open(PROD_LOG_READY, O_CREAT, 0666, 0);
  Prod_log_written = sem_open(PROD_LOG_WRITTEN, O_CREAT, 0666, 1);

  if (Prod_log_ready == SEM_FAILED || Prod_log_written == SEM_FAILED) {
    perror("Producer : SEM_OPEN FAILED...\n");
    exit(1);
  }

  int cons_fd = shm_open(CONS_SHM, O_CREAT | O_RDWR, 0666);
  if (cons_fd == -1) {
    perror("Consumer : SHM_OPEN FAILED...\n");
    exit(1);
  }

  if (ftruncate(cons_fd, sizeof(SharedLog)) == -1) {
    perror("Consumer : FTRUNCATE FAILED...\n");
    exit(1);
  }

  cons_log = mmap(NULL, sizeof(SharedLog), PROT_READ | PROT_WRITE, MAP_SHARED,
                  cons_fd, 0);
  if (cons_log == MAP_FAILED) {
    perror("Consumer : MMAP FAILED...\n");
    exit(1);
  }

  Cons_log_ready = sem_open(CONS_LOG_READY, O_CREAT, 0666, 0);
  Cons_log_written = sem_open(CONS_LOG_WRITTEN, O_CREAT, 0666, 1);

  if (Cons_log_ready == SEM_FAILED || Cons_log_written == SEM_FAILED) {
    perror("Consumer : SEM_OPEN FAILED...\n");
    exit(1);
  }

  int cm_fd = shm_open(CM_SHM, O_CREAT | O_RDWR, 0666);
  if (cm_fd == -1) {
    perror("Car Make : SHM_OPEN FAILED...\n");
    exit(1);
  }

  if (ftruncate(cm_fd, sizeof(SharedLog)) == -1) {
    perror("Car Make : FTRUNCATE FAILED\n");
    exit(1);
  }

  Cm_log = mmap(NULL, sizeof(SharedLog), PROT_READ | PROT_WRITE, MAP_SHARED,
                cm_fd, 0);
  if (Cm_log == MAP_FAILED) {
    perror("Car Make : MMAP FAILED...\n");
    exit(1);
  }

  Cm_log_ready = sem_open(CM_LOG_READY, O_CREAT, 0666, 0);
  Cm_log_written = sem_open(CM_LOG_WRITTEN, O_CREAT, 0666, 1);

  if (Cm_log_ready == SEM_FAILED || Cm_log_written == SEM_FAILED) {
    perror("Car Make : SEM_OPEN FAILED...\n");
    exit(1);
  }
}

void CleanUpFactory() {
  sem_close(Prod_log_ready);
  sem_close(Prod_log_written);
  sem_close(Cons_log_ready);
  sem_close(Cons_log_written);
  sem_unlink(PROD_LOG_READY);
  sem_unlink(PROD_LOG_WRITTEN);
  shm_unlink(PROD_SHM);
  sem_unlink(CONS_LOG_READY);
  sem_unlink(CONS_LOG_WRITTEN);
  shm_unlink(CONS_SHM);
  sem_unlink(CM_LOG_READY);
  sem_unlink(CM_LOG_WRITTEN);
  shm_unlink(CM_SHM);
}

void AddCons() {}

void AddProd() {}

int main() {
  pthread_t prodthread1, prodthread2, consthread1, consthread2, Manager_Thread,
      CarMakeThread;
  /*Manager_Thread*/
  Thread_data prod1 = {Interior, 1, 0};
  Thread_data prod2 = {Exterior, 2, 0};
  Thread_data cons1 = {Interior, 1, 1};
  Thread_data cons2 = {Exterior, 2, 1};

  InitFactory();
  pthread_create(&prodthread1, NULL, Prod, &prod1);
  pthread_create(&prodthread2, NULL, Prod, &prod2);
  pthread_create(&consthread1, NULL, Cons, &cons1);
  pthread_create(&consthread2, NULL, Cons, &cons2);
  pthread_create(&CarMakeThread, NULL, MakeCar, NULL);
  pthread_create(&Manager_Thread, NULL, Manager, NULL);

  pthread_join(prodthread1, NULL);
  pthread_join(prodthread2, NULL);
  pthread_join(consthread1, NULL);
  pthread_join(consthread2, NULL);
  pthread_join(CarMakeThread, NULL);
  pthread_join(Manager_Thread, NULL);

  CleanUpFactory();
  return 0;
}
