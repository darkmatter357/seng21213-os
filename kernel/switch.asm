[BITS 32]

GLOBAL switch_context

; switch_context(old_esp_ptr, new_esp)
;
; Save the current stack pointer and restore the new stack pointer.
;
; Arguments:
;   [esp + 4] = pointer to old ESP storage
;   [esp + 8] = new ESP value

switch_context:
    mov eax, [esp + 4]
    mov edx, [esp + 8]

    mov [eax], esp
    mov esp, edx

    ret
