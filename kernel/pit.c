#include "pit.h"

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

void pit_init(uint32_t frequency)
{
    uint32_t divisor;

    if (frequency == 0)
        frequency = 100;

    divisor = PIT_FREQUENCY / frequency;

    if (divisor > 0xFFFF)
        divisor = 0xFFFF;

    if (divisor == 0)
        divisor = 1;

    /* Channel 0, access low byte then high byte, mode 3. */
    outb(PIT_COMMAND, 0x36);

    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}
