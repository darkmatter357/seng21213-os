#include "scheduler.h"
#include "thread.h"

static pcb_t *current_process = (pcb_t *)0;
static pcb_t *ready_queue = (pcb_t *)0;

static thread_t *current_thread = (thread_t *)0;
static thread_t *thread_queue = (thread_t *)0;

void scheduler_init(void)
{
    current_process = (pcb_t *)0;
    ready_queue = (pcb_t *)0;

    current_thread = (thread_t *)0;
    thread_queue = (thread_t *)0;
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

void scheduler_add_thread(struct thread *thread)
{
    thread_t *tail;

    if (thread == (thread_t *)0)
        return;

    thread->sched_next = (thread_t *)0;

    if (thread_queue == (thread_t *)0) {
        thread_queue = thread;
        return;
    }

    tail = thread_queue;

    while (tail->sched_next != (thread_t *)0)
        tail = tail->sched_next;

    tail->sched_next = thread;
}

pcb_t *scheduler_current(void)
{
    return current_process;
}

thread_t *scheduler_current_thread(void)
{
    return current_thread;
}

pcb_t *scheduler_next(void)
{
    pcb_t *p;

    if (current_process == (pcb_t *)0)
        p = ready_queue;
    else
        p = current_process->next;

    while (p != (pcb_t *)0) {
        if (p->state == READY)
            return p;

        p = p->next;
    }

    p = ready_queue;

    while (p != (pcb_t *)0) {
        if (p->state == READY)
            return p;

        p = p->next;
    }

    return (pcb_t *)0;
}

thread_t *scheduler_next_thread(void)
{
    thread_t *t;

    if (current_thread == (thread_t *)0)
        t = thread_queue;
    else
        t = current_thread->sched_next;

    while (t != (thread_t *)0) {
        if (t->state == THREAD_READY)
            return t;

        t = t->sched_next;
    }

    t = thread_queue;

    while (t != (thread_t *)0) {
        if (t->state == THREAD_READY)
            return t;

        t = t->sched_next;
    }

    return (thread_t *)0;
}

void scheduler_tick(void)
{
    thread_t *next_thread;

    /*
     * If a thread is currently running, return it to READY.
     */
    if (current_thread != (thread_t *)0 &&
        current_thread->state == THREAD_RUNNING) {
        current_thread->state = THREAD_READY;
    }

    /*
     * Prefer runnable kernel threads.
     */
    next_thread = scheduler_next_thread();

    if (next_thread != (thread_t *)0) {
        next_thread->state = THREAD_RUNNING;
        current_thread = next_thread;
        return;
    }

    /*
     * No runnable threads remain.
     * Fall back to the Stage 1 process scheduler.
     */
    current_thread = (thread_t *)0;

    if (current_process != (pcb_t *)0 &&
        current_process->state == RUNNING) {
        current_process->state = READY;
    }

    {
        pcb_t *next = scheduler_next();

        if (next != (pcb_t *)0) {
            next->state = RUNNING;
            current_process = next;
        }
    }
}
