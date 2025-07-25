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

Part_Stack parts;
SharedLog *prod_log = NULL;
SharedLog *cons_log = NULL;
sem_t *Prod_log_ready = NULL;
sem_t *Prod_log_written = NULL;

sem_t *Cons_log_ready = NULL;
sem_t *Cons_log_written = NULL;

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
      pthread_mutex_unlock(&pauseMutex);
      break;
    }
    pthread_mutex_unlock(&pauseMutex);
  }
  return NULL;
}

void InitFactory() {
  parts.top = 0;
  pthread_mutex_init(&parts.mutex, NULL);
  sem_init(&parts.empty, 0, MAX_PARTS);
  sem_init(&parts.full, 0, 0);
  srand(time(NULL));
  pthread_mutex_init(&pauseMutex, NULL);

  int prod_fd = shm_open(PROD_SHM, O_CREAT | O_RDWR, 0666);
  if (prod_fd == -1) {
    perror("shm_open failed");
    exit(1);
  }

  if (ftruncate(prod_fd, sizeof(SharedLog)) == -1) {
    perror("ftruncate failed");
    exit(1);
  }

  prod_log = mmap(NULL, sizeof(SharedLog), PROT_READ | PROT_WRITE, MAP_SHARED,
                  prod_fd, 0);
  if (prod_log == MAP_FAILED) {
    perror("mmap failed");
    exit(1);
  }

  Prod_log_ready = sem_open(PROD_LOG_READY, O_CREAT, 0666, 0);
  Prod_log_written = sem_open(PROD_LOG_WRITTEN, O_CREAT, 0666, 1);

  if (Prod_log_ready == SEM_FAILED || Prod_log_written == SEM_FAILED) {
    perror("sem_open failed");
    exit(1);
  }

  int cons_fd = shm_open(CONS_SHM, O_CREAT | O_RDWR, 0666);
  if (cons_fd == -1) {
    perror("shm_open failed");
    exit(1);
  }

  if (ftruncate(cons_fd, sizeof(SharedLog)) == -1) {
    perror("ftruncate failed");
    exit(1);
  }

  cons_log = mmap(NULL, sizeof(SharedLog), PROT_READ | PROT_WRITE, MAP_SHARED,
                  cons_fd, 0);
  if (cons_log == MAP_FAILED) {
    perror("mmap failed");
    exit(1);
  }

  Cons_log_ready = sem_open(CONS_LOG_READY, O_CREAT, 0666, 0);
  Cons_log_written = sem_open(CONS_LOG_WRITTEN, O_CREAT, 0666, 1);

  if (Cons_log_ready == SEM_FAILED || Cons_log_written == SEM_FAILED) {
    perror("sem_open failed");
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
}

int main() {
  pthread_t prodThread1, consThread1, prodThread2, consThread2, Manager_Thread;
  Thread_data producerData1 = {Exterior, 1};
  Thread_data producerData2 = {Interior, 2};

  InitFactory();
  // pthread_create(&Manager_Thread, NULL, Manager, NULL);
  pthread_create(&prodThread1, NULL, Prod, &producerData1);
  pthread_create(&consThread1, NULL, Cons, NULL);
  pthread_create(&prodThread2, NULL, Prod, &producerData2);
  pthread_create(&consThread2, NULL, Cons, NULL);

  pthread_join(prodThread1, NULL);
  pthread_join(prodThread2, NULL);
  pthread_join(consThread1, NULL);
  pthread_join(consThread2, NULL);
  // pthread_join(Manager_Thread, NULL);

  CleanUpFactory();
  return 0;
}
