#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../include/types.h"

void interrupts_init(void);

/*
 * Called by IRQ0.
 * Receives the current process's saved stack pointer
 * and returns the stack pointer of the process to run next.
 */
uint32_t timer_tick(uint32_t current_esp);

#endif
