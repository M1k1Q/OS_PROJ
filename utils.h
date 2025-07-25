#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PARTS 50
#define MAX_THREADS 10

int numthreads = 0;
int numparts = 0;

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

#endif