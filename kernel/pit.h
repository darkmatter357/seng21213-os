#ifndef PIT_H
#define PIT_H

#include "../include/types.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43

#define PIT_FREQUENCY 1193182

void pit_init(uint32_t frequency);

#endif
