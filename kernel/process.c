#include "process.h"
#include "scheduler.h"

static pcb_t process_table[MAX_PROCESSES];
static uint32_t next_pid = 1;

void process_init(void)
{
    int i;

    next_pid = 1;

    for (i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].state = TERMINATED;
        process_table[i].esp = 0;
        process_table[i].eip = 0;
        process_table[i].next = (pcb_t *)0;
        process_table[i].thread_list = (struct thread *)0;
    }
}

pcb_t *process_create(void (*entry)(void))
{
    int i;

    for (i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == 0) {
            pcb_t *p = &process_table[i];
            uint32_t *stack;

            p->pid = next_pid++;
            p->state = READY;
            p->eip = (uint32_t)entry;
            p->next = (pcb_t *)0;
            p->thread_list = (struct thread *)0;

            stack = &p->stack[STACK_SIZE / 4];

/*
 * Build the stack in reverse order so that ESP points
 * to the first value expected by popa.
 *
 * Final memory layout:
 *
 *   ESP -> EDI
 *           ESI
 *           EBP
 *           ESP dummy
 *           EBX
 *           EDX
 *           ECX
 *           EAX
 *           EIP
 *           CS
 *           EFLAGS
 */
*--stack = 0x202;               /* EFLAGS */
*--stack = 0x08;                /* CS */
*--stack = (uint32_t)entry;     /* EIP */

*--stack = 0;                   /* EAX */
*--stack = 0;                   /* ECX */
*--stack = 0;                   /* EDX */
*--stack = 0;                   /* EBX */
*--stack = 0;                   /* ESP dummy */
*--stack = 0;                   /* EBP */
*--stack = 0;                   /* ESI */
*--stack = 0;                   /* EDI */

p->esp = (uint32_t)stack;

            scheduler_add(p);

            return p;
        }
    }

    return (pcb_t *)0;
}

void process_yield(void)
{
    scheduler_tick();
}

void process_exit(void)
{
    pcb_t *current = scheduler_current();

    if (current != (pcb_t *)0)
        current->state = TERMINATED;

    scheduler_tick();
}
pcb_t *process_get(uint32_t pid)
{
    int i;

    for (i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid)
            return &process_table[i];
    }

    return (pcb_t *)0;
}
