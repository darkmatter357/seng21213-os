[BITS 32]

GLOBAL irq0_handler
EXTERN timer_tick

irq0_handler:
    pusha

    ; Pass the current saved register frame to C.
    push esp
    call timer_tick
    add esp, 4

    ; timer_tick returns the stack pointer to restore in EAX.
    mov esp, eax

    popa
    iretd
