#include "thread.h"
#include "scheduler.h"

static thread_t thread_table[MAX_THREADS];
static uint32_t next_tid = 1;

/*
 * Common wrapper used as the initial thread entry point.
 *
 * The thread function receives its argument and, when it returns,
 * the thread is marked terminated.
 */
static void thread_start(void)
{
    thread_t *t;

    t = scheduler_current_thread();

    if (t != (thread_t *)0 &&
        t->entry != (void (*)(void *))0) {
        t->entry(t->arg);
    }

    thread_exit();

    for (;;)
        __asm__ volatile ("hlt");
}

void thread_init(void)
{
    int i;

    next_tid = 1;

    for (i = 0; i < MAX_THREADS; i++) {
        thread_table[i].tid = 0;
        thread_table[i].state = THREAD_TERMINATED;
        thread_table[i].esp = 0;
        thread_table[i].eip = 0;
        thread_table[i].parent = (pcb_t *)0;
        thread_table[i].entry = (void (*)(void *))0;
        thread_table[i].arg = (void *)0;
        thread_table[i].next = (thread_t *)0;
        thread_table[i].sched_next = (thread_t *)0;
        thread_table[i].wait_next = (thread_t *)0;
    }
}

thread_t *thread_create(void (*fn)(void *), void *arg)
{
    int i;
    pcb_t *parent;

    if (fn == (void (*)(void *))0)
        return (thread_t *)0;

    parent = scheduler_current();

    if (parent == (pcb_t *)0)
        return (thread_t *)0;

    for (i = 0; i < MAX_THREADS; i++) {

        if (thread_table[i].tid == 0) {

            thread_t *t = &thread_table[i];
            uint32_t *stack;

            t->tid = next_tid++;
            t->state = THREAD_READY;
            t->parent = parent;
            t->entry = fn;
            t->arg = arg;
            t->eip = (uint32_t)thread_start;
            t->next = (thread_t *)0;
            t->sched_next = (thread_t *)0;
            t->wait_next = (thread_t *)0;
            /*
             * Construct the same interrupt stack layout used
             * by the Stage 1 IRQ context switch.
             *
             * ESP -> EDI
             *         ESI
             *         EBP
             *         ESP dummy
             *         EBX
             *         EDX
             *         ECX
             *         EAX
             *         EIP
             *         CS
             *         EFLAGS
             */
            stack = &t->stack[THREAD_STACK_SIZE / 4];

            *--stack = 0x202;
            *--stack = 0x08;
            *--stack = (uint32_t)thread_start;

            *--stack = 0;
            *--stack = 0;
            *--stack = 0;
            *--stack = 0;
            *--stack = 0;
            *--stack = 0;
            *--stack = 0;
            *--stack = 0;

            t->esp = (uint32_t)stack;

            /*
             * Attach to parent process.
             */
            t->next = parent->thread_list;
            parent->thread_list = t;

            /*
             * Add to global thread scheduler queue.
             */
            scheduler_add_thread(t);

            return t;
        }
    }

    return (thread_t *)0;
}

void thread_exit(void)
{
    thread_t *t;

    t = scheduler_current_thread();

    if (t != (thread_t *)0)
        t->state = THREAD_TERMINATED;
}

thread_t *thread_current(void)
{
    return scheduler_current_thread();
}
