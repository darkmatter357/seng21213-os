#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

void scheduler_init(void);
pcb_t *scheduler_current(void);
pcb_t *scheduler_next(void);
void scheduler_tick(void);

/* Add a newly created process to the ready queue. */
void scheduler_add(pcb_t *process);

#endif
