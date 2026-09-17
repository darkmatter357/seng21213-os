#include "semaphore.h"
#include "scheduler.h"

void semaphore_init(semaphore_t *sem, int value)
{
    if (sem == (semaphore_t *)0)
        return;

    if (value < 0)
        value = 0;

    sem->count = value;
    sem->waitq = (thread_t *)0;
}

void sem_wait(semaphore_t *sem)
{
    thread_t *current;
    thread_t *tail;

    if (sem == (semaphore_t *)0)
        return;

    current = thread_current();

    if (current == (thread_t *)0)
        return;

    /*
     * A resource is available.
     */
    if (sem->count > 0) {
        sem->count--;
        return;
    }

    /*
     * No resource is available.
     * Block the current thread and add it to the wait queue.
     */
    current->state = THREAD_BLOCKED;
    current->wait_next = (thread_t *)0;

    if (sem->waitq == (thread_t *)0) {
        sem->waitq = current;
    } else {
        tail = sem->waitq;

        while (tail->wait_next != (thread_t *)0)
            tail = tail->wait_next;

        tail->wait_next = current;
    }

    /*
     * Sleep until sem_signal() wakes this thread.
     */
    while (current->state == THREAD_BLOCKED)
        __asm__ volatile ("hlt");

    /*
     * The signal operation transfers the available resource
     * to the awakened thread.
     */
}

void sem_signal(semaphore_t *sem)
{
    thread_t *next;

    if (sem == (semaphore_t *)0)
        return;

    /*
     * Wake the first waiting thread if one exists.
     */
    next = sem->waitq;

    if (next != (thread_t *)0) {
        sem->waitq = next->wait_next;
        next->wait_next = (thread_t *)0;
        next->state = THREAD_READY;
        return;
    }

    /*
     * Otherwise increase the available resource count.
     */
    sem->count++;
}
