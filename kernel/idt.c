#include "idt.h"

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

static void idt_load(void)
{
    __asm__ volatile ("lidtl %0" : : "m"(idt_ptr));
}

void idt_set_gate(uint8_t vector, uint32_t handler)
{
    idt[vector].offset_low = (uint16_t)(handler & 0xFFFF);
    idt[vector].selector = 0x08;
    idt[vector].zero = 0;
    idt[vector].type_attr = 0x8E;
    idt[vector].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
}

void idt_init(void)
{
    int i;

    for (i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt[0];

    idt_load();
}
