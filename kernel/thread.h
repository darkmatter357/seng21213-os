#ifndef THREAD_H
#define THREAD_H

#include "../include/types.h"
#include "process.h"

#define MAX_THREADS 32
#define THREAD_STACK_SIZE 4096

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
} thread_state_t;

typedef struct thread {
    uint32_t tid;
    thread_state_t state;

    uint32_t esp;
    uint32_t eip;

    pcb_t *parent;
    void (*entry)(void *);
    void *arg;

    uint32_t stack[THREAD_STACK_SIZE / 4];

    /* Link in the parent process's thread list. */
    struct thread *next;

    /* Link in the global scheduler thread queue. */
    struct thread *sched_next;
    struct thread *wait_next;
} thread_t;

void thread_init(void);

thread_t *thread_create(void (*fn)(void *), void *arg);

void thread_exit(void);

thread_t *thread_current(void);

#endif
