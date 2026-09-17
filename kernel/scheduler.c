#include "scheduler.h"

static pcb_t *current_process = (pcb_t *)0;
static pcb_t *ready_queue = (pcb_t *)0;

void scheduler_init(void)
{
    current_process = (pcb_t *)0;
    ready_queue = (pcb_t *)0;
}

void scheduler_add(pcb_t *process)
{
    pcb_t *tail;

    if (process == (pcb_t *)0)
        return;

    process->next = (pcb_t *)0;

    if (ready_queue == (pcb_t *)0) {
        ready_queue = process;
        return;
    }

    tail = ready_queue;

    while (tail->next != (pcb_t *)0)
        tail = tail->next;

    tail->next = process;
}

pcb_t *scheduler_current(void)
{
    return current_process;
}

pcb_t *scheduler_next(void)
{
    pcb_t *p;

    /*
     * Start from the process after the current process.
     * On the first scheduling decision, start at the head.
     */
    if (current_process == (pcb_t *)0)
        p = ready_queue;
    else
        p = current_process->next;

    while (p != (pcb_t *)0) {
        if (p->state == READY)
            return p;

        p = p->next;
    }

    /*
     * Wrap around to the beginning of the queue.
     */
    p = ready_queue;

    while (p != (pcb_t *)0) {
        if (p->state == READY)
            return p;

        p = p->next;
    }

    return (pcb_t *)0;
}

void scheduler_tick(void)
{
    pcb_t *next;

    if (current_process != (pcb_t *)0 &&
        current_process->state == RUNNING) {
        current_process->state = READY;
    }

    next = scheduler_next();

    if (next != (pcb_t *)0) {
        next->state = RUNNING;
        current_process = next;
    }
}
