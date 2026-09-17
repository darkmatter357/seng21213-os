/* =============================================================================
 * SENG21213-OS :: Main Kernel
 * File   : kernel/kernel.c
 *
 * Stage 1 integration:
 *   - Process management
 *   - Round-robin scheduler
 *   - Timer-driven scheduling
 *   - Interactive kernel shell
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "process.h"
#include "scheduler.h"
#include "idt.h"
#include "interrupts.h"
#include "../include/types.h"

/* ---------------------------------------------------------------------------
 * Test processes
 * --------------------------------------------------------------------------*/

static void test_process_1(void)
{
    for (;;) {
        __asm__ volatile ("nop");
    }
}

static void test_process_2(void)
{
    for (;;) {
        __asm__ volatile ("nop");
    }
}

/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/

static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_ps(void);
static void cmd_kill(const char *args);

/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers
 * --------------------------------------------------------------------------*/

static int k_strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b)) {
        a++;
        b++;
    }

    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n)
{
    while (n-- && *a && (*a == *b)) {
        a++;
        b++;
    }

    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s)
{
    size_t n = 0;

    while (s[n])
        n++;

    return n;
}

static const char *k_ltrim(const char *s)
{
    while (*s == ' ')
        s++;

    return s;
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/

static void print_splash(void)
{
    vga_clear(VGA_BLACK);

    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color(
        "  SENG21213-OS  |  Computer Architecture & Operating Systems",
        VGA_YELLOW,
        VGA_BLACK
    );

    vga_set_cursor(2, 2);
    vga_puts_color(
        "  Stage 1: Process Management",
        VGA_LIGHT_CYAN,
        VGA_BLACK
    );

    vga_set_cursor(3, 2);
    vga_puts_color(
        "  Faculty of Engineering – Department of Software Engineering",
        VGA_LIGHT_GREY,
        VGA_BLACK
    );

    vga_set_cursor(4, 2);
    vga_puts_color(
        "  Built by students, for students.  Type 'help' to begin.",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );

    vga_set_cursor(5, 2);
    vga_puts_color(
        "  CPU: i686 (32-bit Protected Mode)  |  Display: VGA 80x25",
        VGA_DARK_GREY,
        VGA_BLACK
    );

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    vga_puts(
        "  Welcome! This kernel is now running Stage 1 process management.\n"
    );

    vga_puts(
        "  A PCB, ready queue and round-robin scheduler are active.\n"
    );

    vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/

static void cmd_help(void)
{
    vga_puts_color(
        "\n  SENG21213-OS Shell Commands\n",
        VGA_YELLOW,
        VGA_BLACK
    );

    vga_puts("  ---------------------------------------------\n");
    vga_puts("  help    – Show this help message\n");
    vga_puts("  clear   – Clear the screen\n");
    vga_puts("  about   – About this OS and course\n");
    vga_puts("  echo    – Echo text to screen\n");
    vga_puts("  mem     – Memory map (stub)\n");
    vga_puts("  ps      – List processes\n");
    vga_puts("  kill    – Terminate a process\n");
    vga_puts("\n");
}

static void cmd_clear(void)
{
    vga_clear(VGA_BLACK);
}

static void cmd_about(void)
{
    vga_puts_color(
        "\n  About SENG21213-OS\n",
        VGA_LIGHT_CYAN,
        VGA_BLACK
    );

    vga_puts("  ---------------------------------------------\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 – Sem 2\n");
    vga_puts("  Stage        : Process Management\n\n");
}

static void cmd_echo(const char *args)
{
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_ps(void)
{
    int pid;
    pcb_t *p;

    vga_puts_color(
        "\n  Process List\n",
        VGA_YELLOW,
        VGA_BLACK
    );

    vga_puts("  -----------------------------\n");
    vga_puts("  PID   STATE\n");
    vga_puts("  -----------------------------\n");

    for (pid = 1; pid <= MAX_PROCESSES; pid++) {
        p = process_get(pid);

        if (p == (pcb_t *)0)
            continue;

        vga_printf("  %d     ", p->pid);

        if (p->state == READY)
            vga_puts("READY");
        else if (p->state == RUNNING)
            vga_puts("RUNNING");
        else if (p->state == BLOCKED)
            vga_puts("BLOCKED");
        else if (p->state == TERMINATED)
            vga_puts("TERMINATED");

        vga_puts("\n");
    }

    vga_puts("\n");
}

static void cmd_kill(const char *args)
{
    int pid = 0;
    pcb_t *p;

    args = k_ltrim(args);

    if (*args == '\0') {
        vga_puts_color(
            "  Usage: kill <PID>\n",
            VGA_YELLOW,
            VGA_BLACK
        );
        return;
    }

    while (*args >= '0' && *args <= '9') {
        pid = pid * 10 + (*args - '0');
        args++;
    }

    if (*k_ltrim(args) != '\0') {
        vga_puts_color(
            "  Usage: kill <PID>\n",
            VGA_YELLOW,
            VGA_BLACK
        );
        return;
    }

    p = process_get(pid);

    if (p == (pcb_t *)0) {
        vga_puts_color(
            "  Process not found.\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    if (p == scheduler_current()) {
        vga_puts_color(
            "  Cannot kill the currently running process.\n",
            VGA_YELLOW,
            VGA_BLACK
        );
        return;
    }

    if (p->state == TERMINATED) {
        vga_puts("  Process is already terminated.\n");
        return;
    }

    p->state = TERMINATED;

    vga_puts_color(
        "  Process terminated.\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );
}

static void cmd_mem(void)
{
    vga_puts_color(
        "\n  Memory Map (stub)\n",
        VGA_LIGHT_CYAN,
        VGA_BLACK
    );

    vga_puts("  ---------------------------------------------\n");
    vga_puts("  0x00000000 – 0x000FFFFF  : First 1 MB\n");
    vga_puts("  0x00100000 – 0x00EFFFFF  : Extended memory\n");
    vga_puts("  0x00F00000 – 0x00FFFFFF  : BIOS / ROM area\n");
    vga_puts("  0xB8000    – 0xBFFFF     : VGA frame buffer\n\n");
}

/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/

static char shell_buf[256];
static char prompt[] = "\n  ksh> ";

static void shell_run(void)
{
    vga_puts_color(
        "\n  Kernel Shell ready. Type 'help' for commands.\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );

    while (true) {
        vga_puts_color(
            prompt,
            VGA_LIGHT_GREEN,
            VGA_BLACK
        );

        kb_readline(shell_buf, sizeof(shell_buf));

        {
            const char *cmd = k_ltrim(shell_buf);

            if (k_strlen(cmd) == 0)
                continue;

            if (k_strcmp(cmd, "help") == 0) {
                cmd_help();
                continue;
            }

            if (k_strcmp(cmd, "clear") == 0) {
                cmd_clear();
                continue;
            }

            if (k_strcmp(cmd, "about") == 0) {
                cmd_about();
                continue;
            }

            if (k_strcmp(cmd, "ps") == 0) {
                cmd_ps();
                continue;
            }

            if (k_strcmp(cmd, "mem") == 0) {
                cmd_mem();
                continue;
            }

            if (k_strncmp(cmd, "echo ", 5) == 0) {
                cmd_echo(k_ltrim(cmd + 5));
                continue;
            }

            if (k_strncmp(cmd, "kill ", 5) == 0) {
                cmd_kill(cmd + 5);
                continue;
            }

            if (k_strcmp(cmd, "kill") == 0) {
                cmd_kill("");
                continue;
            }

            vga_puts_color(
                "  Unknown command: ",
                VGA_LIGHT_RED,
                VGA_BLACK
            );

            vga_puts(cmd);
            vga_puts("\n");
        }
    }
}

/* ---------------------------------------------------------------------------
 * Kernel entry point
 * --------------------------------------------------------------------------*/

void kernel_main(void)
{
    vga_init();
    kb_init();

    /*
     * Initialise process management and scheduler.
     */
    process_init();
    scheduler_init();

    /*
     * Create three processes.
     */
    process_create(shell_run);
    process_create(test_process_1);
    process_create(test_process_2);

    /*
     * Initialise interrupt system and timer.
     */
    idt_init();

    print_splash();

    interrupts_init();

    /*
     * The timer interrupt now drives scheduling.
     */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
