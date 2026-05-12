#include "../include/task_common.h"

static volatile int monitor_running = 1;
static void cleanup(int sig) { (void)sig; monitor_running = 0; TASK_END(); exit(0); }

static void read_proc_meminfo(long *total_kb, long *free_kb, long *available_kb,
                               long *buffers_kb, long *cached_kb) {
    *total_kb = *free_kb = *available_kb = *buffers_kb = *cached_kb = 0;
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;
    char label[64]; long val;
    while (fscanf(f, "%63s %ld kB\n", label, &val) == 2) {
        if (!strcmp(label, "MemTotal:"))     *total_kb     = val;
        if (!strcmp(label, "MemFree:"))      *free_kb      = val;
        if (!strcmp(label, "MemAvailable:")) *available_kb = val;
        if (!strcmp(label, "Buffers:"))      *buffers_kb   = val;
        if (!strcmp(label, "Cached:"))       *cached_kb    = val;
    }
    fclose(f);
}

static void draw_bar(long used, long total, int width) {
    int pct    = total > 0 ? (int)((used * 100) / total) : 0;
    int filled = (pct * width) / 100;
    const char *color = pct >= 90 ? "\033[31m" : pct >= 70 ? "\033[33m" : "\033[32m";
    printf("%s", color);
    printf("[");
    for (int i = 0; i < width; i++)
        printf("%s", i < filled ? "█" : "░");
    printf("] \033[0m%3d%%", pct);
}

static void read_lunux_state(int *lunux_used, int *lunux_total, int *proc_count) {
    *lunux_used = *lunux_total = *proc_count = 0;
    int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (fd < 0) return;
    SystemState *s = (SystemState *)mmap(NULL, sizeof(SystemState),
                                          PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (s == MAP_FAILED) return;
    *lunux_used  = s->used_ram_mb;
    *lunux_total = s->total_ram_mb;
    *proc_count  = s->proc_count;
    munmap(s, sizeof(SystemState));
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("RAMMonitor", RAM_MONITOR, 0, 3, QUEUE_PRIO);

    printf("\033[?25l"); /* hide cursor */

    while (monitor_running) {
        long total, free_m, avail, buf, cached;
        read_proc_meminfo(&total, &free_m, &avail, &buf, &cached);

        long used   = total - free_m;
        long cached_real = cached + buf;

        int lu_used, lu_total, lu_procs;
        read_lunux_state(&lu_used, &lu_total, &lu_procs);

        printf("\033[2J\033[H");
        printf("╔══════════════════════════════════════════════════════╗\n");
        printf("║            LunuxOS — RAM Monitor                    ║\n");
        printf("╠══════════════════════════════════════════════════════╣\n");
        printf("║  Updates every 2 seconds │ Ctrl+C to close          ║\n");
        printf("╚══════════════════════════════════════════════════════╝\n\n");

        /* System RAM */
        printf("  ── System RAM (from /proc/meminfo) ──────────────────\n");
        printf("  Total     : %8ld MB\n", total / 1024);
        printf("  Used      : %8ld MB  ", used / 1024);
        draw_bar(used, total, 30); printf("\n");
        printf("  Free      : %8ld MB\n", free_m / 1024);
        printf("  Available : %8ld MB\n", avail / 1024);
        printf("  Cached    : %8ld MB\n", cached_real / 1024);
        printf("  Buffers   : %8ld MB\n\n", buf / 1024);

        /* LunuxOS simulated RAM */
        if (lu_total > 0) {
            printf("  ── LunuxOS Simulated RAM ─────────────────────────────\n");
            printf("  Allocated : %8d MB  ", lu_used);
            draw_bar(lu_used, lu_total, 30); printf("\n");
            printf("  Total     : %8d MB\n", lu_total);
            printf("  Free      : %8d MB\n", lu_total - lu_used);
            printf("  Processes : %8d running\n\n", lu_procs);
        }

        /* History of last 5 readings */
        static long history[5] = {0};
        static int  hist_idx   = 0;
        history[hist_idx % 5] = used / 1024;
        hist_idx++;
        printf("  ── RAM Usage History (last 5 readings, MB) ──────────\n");
        printf("  ");
        for (int i = 0; i < 5; i++) {
            int idx = (hist_idx - 5 + i + 5) % 5;
            printf("[%4ldMB] ", history[idx]);
        }
        printf("\n\n");

        time_t now = time(NULL);
        char tbuf[32];
        strftime(tbuf, sizeof(tbuf), "%H:%M:%S", localtime(&now));
        printf("  Last update: %s\n", tbuf);
        fflush(stdout);
        sleep(2);
    }

    printf("\033[?25h");
    printf("\n  RAM Monitor closed.\n");
    TASK_END();
    return 0;
}
