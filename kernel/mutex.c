#include "mutex.h"
#include "scheduler.h"

void mutex_init(mutex_t *mutex)
{
    if (mutex == (mutex_t *)0)
        return;

    mutex->locked = 0;
    mutex->owner = (thread_t *)0;
    mutex->waitq = (thread_t *)0;
}

void mutex_lock(mutex_t *mutex)
{
    thread_t *current;
    thread_t *tail;

    if (mutex == (mutex_t *)0)
        return;

    current = thread_current();

    if (current == (thread_t *)0)
        return;

    /*
     * Prevent the owner from recursively locking the same mutex.
     */
    if (mutex->owner == current)
        return;

    /*
     * Acquire an unlocked mutex.
     */
    if (mutex->locked == 0) {
        mutex->locked = 1;
        mutex->owner = current;
        return;
    }

    /*
     * Put the current thread on the wait queue.
     */
    current->state = THREAD_BLOCKED;
    current->wait_next = (thread_t *)0;

    if (mutex->waitq == (thread_t *)0) {
        mutex->waitq = current;
    } else {
        tail = mutex->waitq;

        while (tail->wait_next != (thread_t *)0)
            tail = tail->wait_next;

        tail->wait_next = current;
    }

    /*
     * The timer interrupt can now switch to another thread.
     * Keep the thread asleep until mutex_unlock() wakes it.
     */
    while (current->state == THREAD_BLOCKED)
        __asm__ volatile ("hlt");

    /*
     * When awakened by mutex_unlock(), ownership has already
     * been transferred to this thread.
     */
}

void mutex_unlock(mutex_t *mutex)
{
    thread_t *next;
    thread_t *current;

    if (mutex == (mutex_t *)0)
        return;

    current = thread_current();

    if (mutex->owner != current)
        return;

    next = mutex->waitq;

    if (next != (thread_t *)0) {
        mutex->waitq = next->wait_next;
        next->wait_next = (thread_t *)0;

        /*
         * Direct ownership transfer avoids a race between
         * waking the waiter and another thread acquiring it.
         */
        mutex->owner = next;
        mutex->locked = 1;
        next->state = THREAD_READY;
        return;
    }

    mutex->owner = (thread_t *)0;
    mutex->locked = 0;
}
