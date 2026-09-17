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
#include "thread.h"
#include "mutex.h"
#include "semaphore.h"
#include "pmm.h"
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
static void print_uint(uint32_t value)
{
    char buffer[11];
    int i = 10;

    buffer[i] = '\0';

    if (value == 0) {
        vga_puts("0");
        return;
    }

    while (value > 0) {
        buffer[--i] = (char)('0' + (value % 10));
        value /= 10;
    }

    vga_puts(&buffer[i]);
}
static volatile int myglobal = 0;
static volatile int workers_done = 0;
static mutex_t global_mutex;

#define BUFFER_SIZE 5

static int buffer[BUFFER_SIZE];
static int buffer_in = 0;
static int buffer_out = 0;

static semaphore_t items;
static semaphore_t empty;
static semaphore_t buffer_mutex;

static volatile int producer_done = 0;
static volatile int consumer_done = 0;
static void buffer_producer(void *arg)
{
    int i;
    (void)arg;

    for (i = 1; i <= 10; i++) {
        sem_wait(&empty);
        sem_wait(&buffer_mutex);

        buffer[buffer_in] = i;
        buffer_in = (buffer_in + 1) % BUFFER_SIZE;

        vga_puts_color("[BUFFER] Produced: ", VGA_LIGHT_CYAN, VGA_BLACK);
        print_uint((uint32_t)i);
        vga_puts_color("\n", VGA_LIGHT_CYAN, VGA_BLACK);

        sem_signal(&buffer_mutex);
        sem_signal(&items);

        __asm__ volatile ("hlt");
    }

    producer_done++;
    thread_exit();

    for (;;)
        __asm__ volatile ("hlt");
}

static void buffer_consumer(void *arg)
{
    int i;
    int value;
    (void)arg;

    for (i = 0; i < 10; i++) {
        sem_wait(&items);
        sem_wait(&buffer_mutex);

        value = buffer[buffer_out];
        buffer_out = (buffer_out + 1) % BUFFER_SIZE;

        vga_puts_color("[BUFFER] Consumed: ", VGA_LIGHT_GREEN, VGA_BLACK);
        print_uint((uint32_t)value);
        vga_puts_color("\n", VGA_LIGHT_GREEN, VGA_BLACK);

        sem_signal(&buffer_mutex);
        sem_signal(&empty);

        __asm__ volatile ("hlt");
    }

    consumer_done++;
    thread_exit();

    for (;;)
        __asm__ volatile ("hlt");
}

static void race_worker(void *arg)
{
    int i;
    (void)arg;

    for (i = 0; i < 20; i++) {
        int value = myglobal;

/* Deliberately allow another thread to run after the read. */
__asm__ volatile ("hlt");

myglobal = value + 1;
}
    workers_done++;
    thread_exit();
    for (;;)
        __asm__ volatile ("hlt");
}

static void mutex_worker(void *arg)
{
    int i;
    (void)arg;

    for (i = 0; i < 20; i++) {
        mutex_lock(&global_mutex);

        myglobal++;

        mutex_unlock(&global_mutex);

        for (volatile uint32_t delay = 0; delay < 100000; delay++)
            __asm__ volatile ("nop");
    }

        workers_done++;
    thread_exit();
    for (;;)
        __asm__ volatile ("hlt");
}
static void thread_demo(void *arg)
{
    const char *name = (const char *)arg;
    int i;

    for (i = 0; i < 5; i++) {
        vga_puts_color(
            "\n[THREAD] Running: ",
            VGA_LIGHT_CYAN,
            VGA_BLACK
        );

        vga_puts(name);

        vga_puts_color(
            "\n",
            VGA_LIGHT_CYAN,
            VGA_BLACK
        );

        for (volatile uint32_t delay = 0; delay < 500000; delay++)
            __asm__ volatile ("nop");
    }

    thread_exit();

    for (;;)
        __asm__ volatile ("hlt");
}

/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/

static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_memtest(void);
static void cmd_ps(void);
static void cmd_kill(const char *args);
static void cmd_threadtest(void);
static void cmd_race(void);
static void cmd_mutexrace(void);
static void cmd_buffer(void);
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
        "\n  Physical Memory Manager\n",
        VGA_LIGHT_CYAN,
        VGA_BLACK
    );

    vga_puts("  ---------------------------------------------\n");

    vga_puts("  Frame size : ");
    print_uint(PMM_FRAME_SIZE);
    vga_puts(" bytes\n");

    vga_puts("  Total frames: ");
    print_uint(pmm_total_frames());
    vga_puts("\n");

    vga_puts("  Used frames : ");
    print_uint(pmm_used_frames());
    vga_puts("\n");

    vga_puts("  Free frames : ");
    print_uint(pmm_free_frames());
    vga_puts("\n");

    vga_puts("  Total memory: ");
    print_uint(pmm_total_frames() * PMM_FRAME_SIZE / (1024 * 1024));
    vga_puts(" MB\n\n");
}
static void cmd_memtest(void)
{
    uint32_t frame1;
    uint32_t frame2;

    vga_puts_color(
        "\n  PMM Allocation Test\n",
        VGA_LIGHT_CYAN,
        VGA_BLACK
    );

    vga_puts("  Free frames before: ");
    print_uint(pmm_free_frames());
    vga_puts("\n");

    frame1 = pmm_alloc_frame();
    frame2 = pmm_alloc_frame();

    vga_puts("  Allocated frame 1: 0x");
    print_uint(frame1);
    vga_puts("\n");

    vga_puts("  Allocated frame 2: 0x");
    print_uint(frame2);
    vga_puts("\n");

    vga_puts("  Free frames after allocation: ");
    print_uint(pmm_free_frames());
    vga_puts("\n");

    pmm_free_frame(frame1);
    pmm_free_frame(frame2);

    vga_puts("  Free frames after freeing: ");
    print_uint(pmm_free_frames());
    vga_puts("\n");

    vga_puts_color(
        "  PMM allocation test complete.\n\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );
}

/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/

static char shell_buf[256];
static char prompt[] = "\n  ksh> ";

static void cmd_threadtest(void)
{
    thread_t *t1;
    thread_t *t2;

    vga_puts_color(
        "\nStarting Stage 2 thread test...\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );

    t1 = thread_create(thread_demo, (void *)"T1");
    t2 = thread_create(thread_demo, (void *)"T2");

    if (t1 != (thread_t *)0 && t2 != (thread_t *)0) {
        vga_puts_color(
            "Created threads T1 and T2.\n",
            VGA_LIGHT_GREEN,
            VGA_BLACK
        );
    } else {
        vga_puts_color(
            "Thread creation failed.\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
    }
}
static void cmd_race(void)
{
    thread_t *t1;
    thread_t *t2;

        myglobal = 0;
    workers_done = 0;

    vga_puts_color(
        "\nStarting race-condition test...\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );

    t1 = thread_create(race_worker, (void *)0);
    t2 = thread_create(race_worker, (void *)0);

    if (t1 == (thread_t *)0 || t2 == (thread_t *)0) {
        vga_puts_color(
            "Failed to create race threads.\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    vga_puts_color(
        "Two workers created. Expected without mutex: possible lost updates.\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );
     while (workers_done < 2)
        __asm__ volatile ("hlt");

    vga_puts_color("Race test finished. Final myglobal = ", VGA_LIGHT_GREEN, VGA_BLACK);
    print_uint((uint32_t)myglobal);
    vga_puts_color("\n", VGA_LIGHT_GREEN, VGA_BLACK);
}
static void cmd_buffer(void)
{
    thread_t *producer;
    thread_t *consumer;

    buffer_in = 0;
    buffer_out = 0;
    producer_done = 0;
    consumer_done = 0;

    semaphore_init(&items, 0);
    semaphore_init(&empty, BUFFER_SIZE);
    semaphore_init(&buffer_mutex, 1);

    vga_puts_color("\nStarting bounded-buffer test...\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);

    producer = thread_create(buffer_producer, (void *)0);
    consumer = thread_create(buffer_consumer, (void *)0);

    if (producer == (thread_t *)0 || consumer == (thread_t *)0) {
        vga_puts_color("Failed to create buffer threads.\n",
                       VGA_LIGHT_RED, VGA_BLACK);
        return;
    }

    while (producer_done < 1 || consumer_done < 1)
        __asm__ volatile ("hlt");

    vga_puts_color("Bounded-buffer test finished successfully.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);
}
static void cmd_mutexrace(void)
{
    thread_t *t1;
    thread_t *t2;

    myglobal = 0;
workers_done = 0;
    mutex_init(&global_mutex);

    vga_puts_color(
        "\nStarting mutex-protected race test...\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );

    t1 = thread_create(mutex_worker, (void *)0);
    t2 = thread_create(mutex_worker, (void *)0);

    if (t1 == (thread_t *)0 || t2 == (thread_t *)0) {
        vga_puts_color(
            "Failed to create mutex threads.\n",
            VGA_LIGHT_RED,
            VGA_BLACK
        );
        return;
    }

    vga_puts_color(
        "Two workers created. Protected result should be 40.\n",
        VGA_LIGHT_GREEN,
        VGA_BLACK
    );
  while (workers_done < 2)
    __asm__ volatile ("hlt");

vga_puts_color("Mutex test finished. Final myglobal = ", VGA_LIGHT_GREEN, VGA_BLACK);
print_uint((uint32_t)myglobal);
vga_puts_color("\n", VGA_LIGHT_GREEN, VGA_BLACK);
}
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

            if (k_strcmp(cmd, "threadtest") == 0) {
               cmd_threadtest();
               continue;
            }
            if (k_strcmp(cmd, "race") == 0) {
    cmd_race();
    continue;
}

if (k_strcmp(cmd, "mutexrace") == 0) {
    cmd_mutexrace();
    continue;
}
if (k_strcmp(cmd, "buffer") == 0) {
    cmd_buffer();
    continue;
}

            if (k_strcmp(cmd, "mem") == 0) {
                cmd_mem();
                continue;
            }
            if (k_strcmp(cmd, "memtest") == 0) {
                cmd_memtest();
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
    pmm_init();

    /*
     * Initialise process management and scheduler.
     */
    process_init();
    scheduler_init();
    thread_init();
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
