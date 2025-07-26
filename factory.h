#ifndef FACTORY_H
#define FACTORY_H

#include <pthread.h>
#include <semaphore.h>

#define MAX_PARTS 10
#define MAX_THREADS 10
#define LOG_MSG_SIZE 256
#define CM_SHM "/CarMake_log_shm"
#define CM_LOG_READY "/CarMake_sem_log_ready"
#define CM_LOG_WRITTEN "/CarMake_sem_log_written"
#define PROD_SHM "/prod_log_shm"
#define PROD_LOG_READY "/prod_sem_log_ready"
#define PROD_LOG_WRITTEN "/prod_sem_log_written"
#define CONS_SHM "/cons_log_shm"
#define CONS_LOG_READY "/cons_sem_log_ready"
#define CONS_LOG_WRITTEN "/cons_sem_log_written"

typedef enum { Interior, Exterior } part_Type;

typedef struct {
  part_Type type;
  int id;
} Car_Part;

typedef struct {
  Car_Part stacc[MAX_PARTS];
  int top;
  pthread_mutex_t mutex;
  sem_t full;
  sem_t empty;
} Part_Stack;

typedef struct {
  int total_Cars;
  int totalProducedInterior;
  int totalProducedExterior;
  int totalConsumedInterior;
  int totalConsumedExterior;
  pthread_mutex_t statMutex;
} Logger_Data;

typedef struct {
  part_Type type;
  int id;
} Thread_data;

typedef struct {
  char message[LOG_MSG_SIZE];
} SharedLog;

extern volatile __sig_atomic_t isPaused;
extern volatile __sig_atomic_t isShutDown;
extern pthread_mutex_t pauseMutex;
extern Part_Stack produced_list;
extern Part_Stack Ext_list;
extern Part_Stack Int_list;

extern SharedLog *prod_log;
extern SharedLog *cons_log;
extern SharedLog *Cm_log;
extern sem_t *Prod_log_ready;
extern sem_t *Prod_log_written;
extern sem_t *Cons_log_ready;
extern sem_t *Cons_log_written;
extern sem_t *Cm_log_ready;
extern sem_t *Cm_log_written;

void *Prod(void *arg);
void *Cons();
void *Manager();
void AddProd();
void AddCons();
void MakeCar();
void InitFactory();
void CleanUpFactory();

#endif
