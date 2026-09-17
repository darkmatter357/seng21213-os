#include "pic.h"

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

void pic_init(void)
{
    uint8_t mask1;
    uint8_t mask2;

    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);

    /* Start initialization sequence. */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    /* Remap IRQs:
       Master PIC: IRQ0-7  -> vectors 32-39
       Slave PIC:  IRQ8-15 -> vectors 40-47 */
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    /* Tell each PIC about the other. */
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    /* 8086/88 mode. */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    /*
     * Enable only IRQ0 (PIT timer) for Stage 1.
     * Keep all other hardware IRQs masked for now.
     */
    (void)mask1;
    (void)mask2;

    outb(PIC1_DATA, 0xFE);
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);

    outb(PIC1_COMMAND, PIC_EOI);
}
