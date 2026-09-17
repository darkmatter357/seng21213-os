#include "interrupts.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "scheduler.h"
#include "process.h"

extern void irq0_handler(void);

static volatile uint32_t timer_ticks = 0;

uint32_t timer_tick(uint32_t current_esp)
{
    pcb_t *current;
    pcb_t *next;

    timer_ticks++;

    current = scheduler_current();

    /*
     * Save the interrupted process's CPU context.
     */
    if (current != (pcb_t *)0)
        current->esp = current_esp;

    /*
     * Select the next READY process.
     */
    scheduler_tick();

    next = scheduler_current();

    pic_send_eoi(0);

    /*
     * If a process is available, restore its saved context.
     */
    if (next != (pcb_t *)0)
        return next->esp;

    /*
     * Otherwise continue the interrupted context.
     */
    return current_esp;
}

void interrupts_init(void)
{
    /*
     * IRQ0 = interrupt vector 32 after PIC remapping.
     */
    idt_set_gate(32, (uint32_t)irq0_handler);

    pic_init();
    pit_init(100);

    __asm__ volatile ("sti");
}
