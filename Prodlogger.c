#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "factory.h"  // Ensure this contains SharedLog and LOG_MSG_SIZE

#define LOG_READY "/prod_sem_log_ready"
#define LOG_WRITTEN "/prod_sem_log_written"
#define LOG_SHM "/prod_log_shm"

int main() {
  // Open shared memory with read/write so mmap doesn't break
  int shm_fd = shm_open(LOG_SHM, O_RDWR, 0666);
  if (shm_fd == -1) {
    perror("shm_open failed");
    exit(EXIT_FAILURE);
  }

  // Map with read + write access to avoid segmentation faults
  SharedLog *log = mmap(NULL, sizeof(SharedLog), PROT_READ | PROT_WRITE,
                        MAP_SHARED, shm_fd, 0);
  if (log == MAP_FAILED) {
    perror("mmap failed");
    exit(EXIT_FAILURE);
  }

  // Open semaphores
  sem_t *log_ready = sem_open(LOG_READY, 0);
  sem_t *log_written = sem_open(LOG_WRITTEN, 0);
  if (log_ready == SEM_FAILED || log_written == SEM_FAILED) {
    perror("sem_open failed");
    exit(EXIT_FAILURE);
  }

  // Log loop
  while (1) {
    sem_wait(log_ready);
    printf("logger : %s\n", log->message);
    sem_post(log_written);
  }

  return 0;
}
