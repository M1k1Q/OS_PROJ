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

#define LOG_READY "/CarMake_sem_log_ready"
#define LOG_WRITTEN "/CarMake_sem_log_written"
#define LOG_SHM "/CarMake_log_shm"

typedef struct {
  char msg[100];
} logmsg;

int main() {
  int shm_fd = shm_open(LOG_SHM, O_RDONLY, 0666);
  logmsg *log;

  sem_t *log_ready = sem_open(LOG_READY, 0);
  sem_t *log_written = sem_open(LOG_WRITTEN, 0);

  while (1) {
    sem_wait(log_ready);
    printf("logger : %s\n", log->msg);
    sem_post(log_written);
  }

  return 0;
}