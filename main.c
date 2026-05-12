#include "include/lunuxos.h"
#include "include/logger.h"
#include "include/kernel.h"
#include "include/scheduler.h"
#include "include/memory.h"
#include "include/ipc.h"
#include "include/sync.h"
#include "include/deadlock.h"

/* ════════════════════════════════════════════════════════════════
   Global state pointers
   ════════════════════════════════════════════════════════════════ */
static SystemState  *g_state = NULL;
static MemoryManager g_mm;

/* ════════════════════════════════════════════════════════════════
   Task descriptor table
   ════════════════════════════════════════════════════════════════ */
typedef struct {
    int         id;
    const char *name;
    const char *binary;
    int         ram_mb;
    QueueLevel  queue;
    int         priority;
} TaskDesc;

static const TaskDesc TASKS[] = {
    { 1, "Calculator",       "build/task_calculator",    RAM_CALCULATOR,  QUEUE_RR,   7},
    { 2, "Notepad",          "build/task_notepad",       RAM_NOTEPAD,     QUEUE_RR,   6},
    { 3, "Clock",            "build/task_clock",         RAM_CLOCK,       QUEUE_PRIO, 5},
    { 4, "Calendar",         "build/task_calendar",      RAM_CALENDAR,    QUEUE_FCFS, 4},
    { 5, "File Creator",     "build/task_file_creator",  RAM_FILE_OPS,    QUEUE_FCFS, 5},
    { 6, "File Copier",      "build/task_file_copier",   RAM_FILE_OPS,    QUEUE_FCFS, 5},
    { 7, "File Mover",       "build/task_file_mover",    RAM_FILE_OPS,    QUEUE_FCFS, 5},
    { 8, "File Deleter",     "build/task_file_deleter",  RAM_FILE_OPS,    QUEUE_FCFS, 5},
    { 9, "File Info",        "build/task_file_info",     RAM_FILE_OPS,    QUEUE_FCFS, 4},
    {10, "Mini Shell",       "build/task_shell",         RAM_SHELL,       QUEUE_RR,   8},
    {11, "Text Editor",      "build/task_text_editor",   RAM_EDITOR,      QUEUE_RR,   7},
    {12, "Beep/Music",       "build/task_beep",          RAM_BEEP,        QUEUE_PRIO, 2},
    {13, "Stopwatch",        "build/task_stopwatch",     RAM_STOPWATCH,   QUEUE_RR,   6},
    {14, "Timer",            "build/task_timer",         RAM_TIMER,       QUEUE_RR,   6},
    {15, "Typing Tester",    "build/task_typing",        RAM_TYPING,      QUEUE_RR,   6},
    {16, "Number Guessing",  "build/task_guessing",      RAM_GUESSING,    QUEUE_RR,   5},
    {17, "Tic Tac Toe",      "build/task_tictactoe",     RAM_TICTACTOE,   QUEUE_RR,   5},
    {18, "Minesweeper",      "build/task_minesweeper",   RAM_MINESWEEPER, QUEUE_RR,   5},
    {19, "RAM Monitor",      "build/task_ram_monitor",   RAM_MONITOR,     QUEUE_PRIO, 3},
    {20, "CPU Monitor",      "build/task_cpu_monitor",   RAM_MONITOR,     QUEUE_PRIO, 3},
};
#define NUM_TASKS (int)(sizeof(TASKS)/sizeof(TASKS[0]))

/* ════════════════════════════════════════════════════════════════
   Boot animation
   ════════════════════════════════════════════════════════════════ */
static void boot_screen(void) {
    printf("\033[2J\033[H");
    printf("\033[1;36m");
    printf("\n\n");
    printf("  ██╗     ██╗   ██╗███╗   ██╗██╗   ██╗██╗  ██╗ ██████╗ ███████╗\n");
    printf("  ██║     ██║   ██║████╗  ██║██║   ██║╚██╗██╔╝██╔═══██╗██╔════╝\n");
    printf("  ██║     ██║   ██║██╔██╗ ██║██║   ██║ ╚███╔╝ ██║   ██║███████╗\n");
    printf("  ██║     ██║   ██║██║╚██╗██║██║   ██║ ██╔██╗ ██║   ██║╚════██║\n");
    printf("  ███████╗╚██████╔╝██║ ╚████║╚██████╔╝██╔╝ ██╗╚██████╔╝███████║\n");
    printf("  ╚══════╝ ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚══════╝\n");
    printf("\033[0m");
    printf("\n\033[1;33m");
    printf("                  Operating System Simulator v1.0.0\n");
    printf("                  Built with C │ POSIX │ pthreads\n");
    printf("\033[0m\n");

    const char *steps[] = {
        "Initializing kernel",
        "Loading process table",
        "Starting scheduler",
        "Initializing memory manager",
        "Setting up IPC channels",
        "Loading synchronization primitives",
        "Starting deadlock detector",
        "Loading task registry",
        "Starting system logger",
        "LunuxOS ready",
    };

    for (int i = 0; i < 10; i++) {
        printf("  \033[32m[  OK  ]\033[0m %s", steps[i]);
        fflush(stdout);
        usleep(200000);
        /* Animate dots */
        for (int d = 0; d < 3; d++) {
            printf(".");
            fflush(stdout);
            usleep(80000);
        }
        printf("\n");
    }
    printf("\n");
    sleep(1);
}

/* ════════════════════════════════════════════════════════════════
   System configuration prompt
   ════════════════════════════════════════════════════════════════ */
static void configure_system(int *ram_mb, int *storage_gb, int *cores) {
    printf("\033[2J\033[H");
    printf("  ╔════════════════════════════════════════╗\n");
    printf("  ║     LunuxOS — System Configuration    ║\n");
    printf("  ╚════════════════════════════════════════╝\n\n");

    char buf[64];
    /* RAM */
    while (1) {
        printf("  Enter RAM size in MB (min 256, max 32768): ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) exit(1);
        *ram_mb = atoi(buf);
        if (*ram_mb >= 256 && *ram_mb <= 32768) break;
        printf("  Invalid value. Please enter between 256 and 32768.\n");
    }

    /* Storage */
    while (1) {
        printf("  Enter Storage size in GB (min 1, max 1000): ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) exit(1);
        *storage_gb = atoi(buf);
        if (*storage_gb >= 1 && *storage_gb <= 1000) break;
        printf("  Invalid value. Please enter between 1 and 1000.\n");
    }

    /* Cores */
    while (1) {
        printf("  Enter number of CPU cores (min 1, max 128): ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) exit(1);
        *cores = atoi(buf);
        if (*cores >= 1 && *cores <= 128) break;
        printf("  Invalid value. Please enter between 1 and 128.\n");
    }

    printf("\n  Configuration:\n");
    printf("    RAM     : %d MB\n", *ram_mb);
    printf("    Storage : %d GB\n", *storage_gb);
    printf("    Cores   : %d\n\n", *cores);
    sleep(1);
}

/* ════════════════════════════════════════════════════════════════
   Print the main task menu
   ════════════════════════════════════════════════════════════════ */
static void print_task_menu(void) {
    printf("\033[2J\033[H");
    printf("  ╔════════════════════════════════════════════════════════════╗\n");
    printf("  ║                   LunuxOS — Task Menu                     ║\n");
    printf("  ╠════════════════════════════════════════════════════════════╣\n");
    printf("  ║  %-3s  %-20s  %-10s  %-6s  %s\n",
           "#", "Task Name", "Queue", "RAM", "Priority");
    printf("  ╠════════════════════════════════════════════════════════════╣\n");
    for (int i = 0; i < NUM_TASKS; i++) {
        const char *qname = TASKS[i].queue == QUEUE_RR   ? "RoundRobin" :
                            TASKS[i].queue == QUEUE_FCFS  ? "FCFS" : "Priority";
        printf("  ║  %-3d  %-20s  %-10s  %-4dMB  %d\n",
               TASKS[i].id, TASKS[i].name, qname, TASKS[i].ram_mb, TASKS[i].priority);
    }
    printf("  ╠════════════════════════════════════════════════════════════╣\n");
    printf("  ║  System Commands:                                          ║\n");
    printf("  ║   ps     – list processes   queues – show scheduler queues ║\n");
    printf("  ║   mem    – memory status    res    – resource status       ║\n");
    printf("  ║   dl     – deadlock check   ipc    – IPC demo             ║\n");
    printf("  ║   sync   – sync demo        km     – kernel mode          ║\n");
    printf("  ║   log    – view system log  shutdown – shutdown LunuxOS   ║\n");
    printf("  ╚════════════════════════════════════════════════════════════╝\n");

    /* Resource bar */
    if (g_state) {
        int pct = g_state->total_ram_mb > 0 ?
                  (g_state->used_ram_mb * 100) / g_state->total_ram_mb : 0;
        printf("  RAM: %dMB/%dMB (%d%%)  Storage: %dMB/%dMB  Procs: %d\n\n",
               g_state->used_ram_mb, g_state->total_ram_mb, pct,
               g_state->used_storage_mb, g_state->total_storage_mb,
               g_state->proc_count);
    }
    printf("  LunuxOS> ");
    fflush(stdout);
}

/* ════════════════════════════════════════════════════════════════
   Launch a task in the SAME terminal (fork + exec)
   ════════════════════════════════════════════════════════════════ */
static pid_t launch_task(int task_idx) {
    if (task_idx < 0 || task_idx >= NUM_TASKS) return -1;

    const TaskDesc *t = &TASKS[task_idx];

    /* Resource pre-check */
    if (g_state) {
        if (g_state->used_ram_mb + t->ram_mb > g_state->total_ram_mb) {
            printf("\n  \033[31m[LunuxOS] ERROR: Insufficient RAM for '%s'.\033[0m\n", t->name);
            printf("  Requested: %dMB | Available: %dMB\n\n",
                   t->ram_mb, g_state->total_ram_mb - g_state->used_ram_mb);
            LOG_WARN("Resource denied for task '%s'", t->name);
            return -1;
        }
    }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }

    if (pid == 0) {
        /* Child process */
        execl(t->binary, t->binary, (char *)NULL);
        fprintf(stderr, "\n[LunuxOS] EXEC FAILED: could not run '%s'\n", t->binary);
        fprintf(stderr, "  Ensure 'make' was run in the LunuxOS directory.\n");
        exit(1);
    }

    printf("\n  \033[32m[LunuxOS] Launched '%s' (PID: %d)\033[0m\n\n", t->name, (int)pid);
    LOG_INFO("Launched task '%s' PID=%d", t->name, (int)pid);

    /* Wait for the child (blocks until task exits) */
    int status;
    waitpid(pid, &status, 0);
    LOG_INFO("Task '%s' PID=%d exited, status=%d", t->name, (int)pid, WEXITSTATUS(status));
    return pid;
}

/* ════════════════════════════════════════════════════════════════
   Kernel mode sub-menu
   ════════════════════════════════════════════════════════════════ */
static void kernel_mode_menu(void) {
    g_state->kernel_mode = 1;
    char buf[128];
    printf("\n  \033[1;31m[KERNEL MODE]\033[0m  Commands: kill <pid>, min <pid>, res <pid>, exit\n\n");
    while (1) {
        printf("  kernel# ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) break;
        buf[strcspn(buf, "\n")] = '\0';

        pid_t pid;
        if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) break;
        if (sscanf(buf, "kill %d", (int *)&pid) == 1) {
            kernel_kill_process(g_state, pid);
        } else if (sscanf(buf, "min %d", (int *)&pid) == 1) {
            kernel_minimize_process(g_state, pid);
        } else if (sscanf(buf, "res %d", (int *)&pid) == 1) {
            kernel_resume_process(g_state, pid);
        } else if (strcmp(buf, "ps") == 0) {
            kernel_list_processes(g_state);
        } else if (strcmp(buf, "help") == 0) {
            printf("  kill <pid>  – force kill process\n");
            printf("  min  <pid>  – minimize (SIGSTOP)\n");
            printf("  res  <pid>  – resume   (SIGCONT)\n");
            printf("  ps          – list all processes\n");
            printf("  exit        – leave kernel mode\n\n");
        } else {
            printf("  Unknown kernel command. Type 'help'.\n");
        }
    }
    g_state->kernel_mode = 0;
    printf("  [Left kernel mode]\n\n");
}

/* ════════════════════════════════════════════════════════════════
   Show last N lines of the log file
   ════════════════════════════════════════════════════════════════ */
static void show_log(int lines) {
    FILE *f = fopen(LOG_FILE, "r");
    if (!f) { printf("  No log file found at %s\n\n", LOG_FILE); return; }

    /* Read all lines into a circular buffer */
    char buf[512];
    char log_buf[200][512];
    int  total = 0;
    while (fgets(buf, sizeof(buf), f)) {
        strncpy(log_buf[total % 200], buf, 511);
        total++;
    }
    fclose(f);

    int start = total > lines ? total - lines : 0;
    printf("\n  ── System Log (last %d lines) ────────────────\n", lines);
    for (int i = start; i < total; i++)
        printf("  %s", log_buf[i % 200]);
    printf("  ─────────────────────────────────────────────\n\n");
}

/* ════════════════════════════════════════════════════════════════
   Graceful shutdown
   ════════════════════════════════════════════════════════════════ */
static void do_shutdown(void) {
    printf("\033[2J\033[H");
    printf("\n\033[1;33m");
    printf("  ┌──────────────────────────────────────────┐\n");
    printf("  │      LunuxOS — Shutting Down...          │\n");
    printf("  └──────────────────────────────────────────┘\n");
    printf("\033[0m\n");

    /* Terminate all running tasks */
    if (g_state) {
        printf("  Terminating all processes...\n");
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (g_state->proc_table[i].valid) {
                pid_t pid = g_state->proc_table[i].pid;
                if (pid != getpid()) {
                    kill(pid, SIGTERM);
                    printf("  Terminated PID %d (%s)\n",
                           (int)pid, g_state->proc_table[i].name);
                }
            }
        }
        sleep(1);
    }

    const char *steps[] = {
        "Stopping scheduler",
        "Flushing logs",
        "Freeing memory",
        "Releasing IPC resources",
        "Destroying kernel",
    };
    for (int i = 0; i < 5; i++) {
        printf("  \033[33m[....]\033[0m %s...\n", steps[i]);
        fflush(stdout);
        usleep(300000);
    }

    scheduler_stop();
    LOG_INFO("LunuxOS shutdown initiated by user");
    if (g_state) kernel_shutdown(g_state);

    printf("\n\033[1;36m");
    printf("  Thank you for using LunuxOS v1.0.0\n");
    printf("  Goodbye.\n");
    printf("\033[0m\n");
    exit(0);
}

/* ════════════════════════════════════════════════════════════════
   Signal handler for the main process
   ════════════════════════════════════════════════════════════════ */
static void main_sig_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n  [LunuxOS] Ctrl+C caught. Use 'shutdown' to quit.\n  LunuxOS> ");
        fflush(stdout);
    }
}

/* ════════════════════════════════════════════════════════════════
   Background monitor: reaps zombie children
   ════════════════════════════════════════════════════════════════ */
static void *reaper_thread(void *arg) {
    (void)arg;
    while (g_state && g_state->kernel_running) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            LOG_INFO("Reaped zombie PID=%d", (int)pid);
        }
        sleep(2);
    }
    return NULL;
}

/* ════════════════════════════════════════════════════════════════
   MAIN
   ════════════════════════════════════════════════════════════════ */
int main(void) {
    signal(SIGINT,  main_sig_handler);
    signal(SIGCHLD, SIG_DFL);

    /* 1. Boot screen */
    boot_screen();

    /* 2. Configure hardware */
    int ram_mb, storage_gb, cores;
    configure_system(&ram_mb, &storage_gb, &cores);

    /* 3. Init kernel */
    g_state = kernel_init(ram_mb, storage_gb, cores);
    if (!g_state) {
        fprintf(stderr, "[LunuxOS] FATAL: kernel_init failed\n");
        return 1;
    }

    /* 4. Init memory manager */
    memory_init(&g_mm, ram_mb);

    /* 5. Start scheduler */
    scheduler_start(g_state);

    /* 6. Start background reaper */
    pthread_t reaper;
    pthread_create(&reaper, NULL, reaper_thread, NULL);
    pthread_detach(reaper);

    LOG_INFO("LunuxOS v%s started — RAM=%dMB Storage=%dGB Cores=%d",
             OS_VERSION, ram_mb, storage_gb, cores);

    /* 7. Main command loop */
    char input[256];
    while (1) {
        print_task_menu();

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = '\0';

        if (!strlen(input)) continue;

        /* ── Numeric: launch task ── */
        int task_num = atoi(input);
        if (task_num >= 1 && task_num <= NUM_TASKS) {
            launch_task(task_num - 1);
            continue;
        }

        /* ── String commands ── */
        if (strcmp(input, "shutdown") == 0) {
            do_shutdown();

        } else if (strcmp(input, "ps") == 0) {
            kernel_list_processes(g_state);
            printf("  Press Enter to continue...");
            fflush(stdout); getchar();

        } else if (strcmp(input, "queues") == 0) {
            scheduler_print_queues(g_state);
            printf("  Press Enter to continue...");
            fflush(stdout); getchar();

        } else if (strcmp(input, "mem") == 0) {
            memory_print_status(&g_mm);
            kernel_print_resources(g_state);
            printf("  Press Enter to continue...");
            fflush(stdout); getchar();

        } else if (strcmp(input, "res") == 0) {
            printf("\n  ── Resource Status ─────────────────────\n");
            kernel_print_resources(g_state);
            printf("  Deadlocks detected so far: %d\n\n", g_state->deadlock_count);
            printf("  Press Enter to continue...");
            fflush(stdout); getchar();

        } else if (strcmp(input, "dl") == 0) {
            deadlock_run_check(g_state);
            printf("  Press Enter to continue...");
            fflush(stdout); getchar();

        } else if (strcmp(input, "ipc") == 0) {
            ipc_run_demo();
            printf("  Press Enter to continue...");
            fflush(stdout); getchar();

        } else if (strcmp(input, "sync") == 0) {
            printf("\n  Choose sync demo:\n");
            printf("  1. Producer-Consumer\n");
            printf("  2. Reader-Writer\n");
            printf("  3. Dining Philosophers\n");
            printf("  Choice: ");
            fflush(stdout);
            char sc[8]; fgets(sc, sizeof(sc), stdin);
            int sc_choice = atoi(sc);
            switch (sc_choice) {
                case 1: sync_producer_consumer();   break;
                case 2: sync_reader_writer();        break;
                case 3: sync_dining_philosopher();   break;
                default: printf("  Invalid.\n");
            }
            printf("  Press Enter to continue...");
            fflush(stdout); getchar();

        } else if (strcmp(input, "km") == 0) {
            kernel_mode_menu();

        } else if (strcmp(input, "log") == 0) {
            show_log(30);
            printf("  Press Enter to continue...");
            fflush(stdout); getchar();

        } else if (strcmp(input, "help") == 0 || strcmp(input, "?") == 0) {
            printf("\n  Type a number (1-%d) to launch that task.\n", NUM_TASKS);
            printf("  Commands: ps, queues, mem, res, dl, ipc, sync, km, log, shutdown\n\n");
            printf("  Press Enter to continue...");
            fflush(stdout); getchar();

        } else {
            printf("\n  Unknown command: '%s'. Type a task number or 'help'.\n\n", input);
            printf("  Press Enter to continue...");
            fflush(stdout); getchar();
        }
    }

    do_shutdown();
    return 0;
}
