#include "interrupts.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "scheduler.h"
#include "process.h"
#include "thread.h"

extern void irq0_handler(void);

static volatile uint32_t timer_ticks = 0;

uint32_t timer_tick(uint32_t current_esp)
{
    pcb_t *current_process;
    thread_t *current_thread;
    thread_t *next_thread;

    timer_ticks++;

    /*
     * Save the currently running thread's context when a thread
     * is active. Otherwise save the current process context.
     */
    current_thread = scheduler_current_thread();

    if (current_thread != (thread_t *)0) {
        current_thread->esp = current_esp;
    } else {
        current_process = scheduler_current();

        if (current_process != (pcb_t *)0)
            current_process->esp = current_esp;
    }

    /*
     * Select the next runnable execution context.
     */
    scheduler_tick();

    next_thread = scheduler_current_thread();

    pic_send_eoi(0);

    /*
     * A thread has priority when threads are present.
     */
    if (next_thread != (thread_t *)0)
        return next_thread->esp;

    /*
     * Otherwise continue using the process scheduler.
     */
    current_process = scheduler_current();

    if (current_process != (pcb_t *)0)
        return current_process->esp;

    return current_esp;
}

void interrupts_init(void)
{
    idt_set_gate(32, (uint32_t)irq0_handler);

    pic_init();
    pit_init(100);

    __asm__ volatile ("sti");
}
