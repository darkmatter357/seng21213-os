#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "thread.h"

typedef struct semaphore {
    int count;
    thread_t *waitq;
} semaphore_t;

void semaphore_init(semaphore_t *sem, int value);
void sem_wait(semaphore_t *sem);
void sem_signal(semaphore_t *sem);

#endif
