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

pthread_t prod_threads[MAX_THREADS];
pthread_t cons_threads[MAX_THREADS];
Thread_data prod_data[MAX_THREADS];
Thread_data cons_data[MAX_THREADS];
int prod_count = 0;
int cons_count = 0;

void *Manager() {
  char cmd;
  while (1) {
    printf("Enter command:\n");
    printf(
        "p = pause | r = resume | i = add Interior Producer | e = add Exterior "
        "Producer\n");
    printf(
        "I = add Interior Consumer | E = add Exterior Consumer | q = quit\n");
    fflush(stdout);
    cmd = getchar();
    getchar();

    pthread_mutex_lock(&pauseMutex);
    switch (cmd) {
      case 'p':
        isPaused = 1;
        system("clear");
        printf("Paused\n");
        break;
      case 'r':
        isPaused = 0;
        system("clear");
        printf("Resumed\n");
        break;
      case 'i':
        system("clear");
        AddProd(Interior);
        break;
      case 'e':
        system("clear");
        AddProd(Exterior);
        break;
      case 'I':
        system("clear");
        AddCons(Interior);
        break;
      case 'E':
        system("clear");
        AddCons(Exterior);
        break;
      case 'q':
        isShutDown = 1;
        system("clear");
        printf("Shutting down...\n");
        prod_log->shutdown = 1;
        cons_log->shutdown = 1;
        Cm_log->shutdown = 1;

        int total = prod_count + cons_count + 4;
        for (int i = 0; i < total; i++) {
          sem_post(&Ext_list.full);
          sem_post(&Int_list.full);
          sem_post(&produced_Int_list.full);
          sem_post(&produced_Ext_list.full);
          sem_post(&Ext_list.empty);
          sem_post(&Int_list.empty);
        }
        sem_post(Prod_log_ready);
        sem_post(Cons_log_ready);
        sem_post(Cm_log_ready);
        pthread_mutex_unlock(&pauseMutex);
        return NULL;
      default:
        printf("Invalid command.\n");
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
  sem_close(Cm_log_ready);
  sem_close(Cm_log_written);
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

void AddCons(part_Type type) {
  if (cons_count >= MAX_THREADS) {
    printf("Maximum producer threads\n");
    return;
  }

  cons_data[cons_count] =
      (Thread_data){.type = type, .id = cons_count + 1, .is_consumer = 0};
  pthread_create(&cons_threads[cons_count], NULL, Cons, &cons_data[cons_count]);
  printf("Thread created , ID : %d , Type : Consumer for %s parts\n",
         cons_count + 1, type == Interior ? "Interior" : "Exterior");
  cons_count++;
}

void AddProd(part_Type type) {
  if (prod_count >= MAX_THREADS) {
    printf("Maximum producer threads\n");
    return;
  }

  prod_data[prod_count] =
      (Thread_data){.type = type, .id = prod_count + 1, .is_consumer = 0};
  pthread_create(&prod_threads[prod_count], NULL, Prod, &prod_data[prod_count]);
  printf("Thread created , ID : %d , Type : Producer for %s parts\n",
         prod_count + 1, type == Interior ? "Interior" : "Exterior");
  prod_count++;
}

int main() {
  pthread_t Manager_Thread, CarMakeThread;

  // Initial producer/consumer data
  Thread_data prod1 = {Interior, 1, 0};
  Thread_data prod2 = {Exterior, 2, 0};
  Thread_data cons1 = {Interior, 1, 1};
  Thread_data cons2 = {Exterior, 2, 1};

  InitFactory();

  // Start initial producer and consumer threads
  prod_data[0] = prod1;
  prod_data[1] = prod2;
  cons_data[0] = cons1;
  cons_data[1] = cons2;

  pthread_create(&prod_threads[0], NULL, Prod, &prod_data[0]);
  pthread_create(&prod_threads[1], NULL, Prod, &prod_data[1]);
  pthread_create(&cons_threads[0], NULL, Cons, &cons_data[0]);
  pthread_create(&cons_threads[1], NULL, Cons, &cons_data[1]);
  prod_count = 2;
  cons_count = 2;

  pthread_create(&CarMakeThread, NULL, MakeCar, NULL);
  pthread_create(&Manager_Thread, NULL, Manager, NULL);

  // Wait for all producer threads
  for (int i = 0; i < prod_count; i++) {
    pthread_join(prod_threads[i], NULL);
  }

  // Wait for all consumer threads
  for (int i = 0; i < cons_count; i++) {
    pthread_join(cons_threads[i], NULL);
  }

  pthread_join(CarMakeThread, NULL);
  pthread_join(Manager_Thread, NULL);

  CleanUpFactory();
  return 0;
}
