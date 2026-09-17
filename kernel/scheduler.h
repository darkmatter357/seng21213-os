#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

struct thread;

void scheduler_init(void);

pcb_t *scheduler_current(void);
pcb_t *scheduler_next(void);

void scheduler_tick(void);
void scheduler_add(pcb_t *process);

/* Thread support. */
void scheduler_add_thread(struct thread *thread);
struct thread *scheduler_current_thread(void);
struct thread *scheduler_next_thread(void);

#endif
