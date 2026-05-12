#include "../include/task_common.h"

static volatile int timer_running = 1;
static void cleanup(int sig) { (void)sig; timer_running = 0; TASK_END(); exit(0); }

static void draw_progress(int remaining, int total) {
    int bar_w  = 30;
    int filled = total > 0 ? ((total - remaining) * bar_w) / total : bar_w;
    int pct    = total > 0 ? ((total - remaining) * 100) / total : 100;
    printf("[");
    for (int i = 0; i < bar_w; i++)
        printf("%s", i < filled ? "\033[42m \033[0m" : "\033[41m \033[0m");
    printf("] %3d%%", pct);
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("Timer", RAM_TIMER, 0, 6, QUEUE_RR);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════╗\n");
    printf("║        LunuxOS — Timer               ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    int h, m, s, total;
    while (1) {
        printf("  Enter time (HH MM SS): ");
        fflush(stdout);
        char line[64];
        if (!fgets(line, sizeof(line), stdin)) { TASK_END(); return 0; }
        if (sscanf(line, "%d %d %d", &h, &m, &s) == 3) break;
        if (sscanf(line, "%d", &s) == 1 && h == 0) { m = 0; h = 0; break; }
        printf("  Invalid. Try:  0 5 30  (for 5min 30sec)\n");
    }

    total = h * 3600 + m * 60 + s;
    if (total <= 0) { printf("  Invalid time.\n"); TASK_END(); return 0; }

    printf("\n");
    int remaining = total;

    while (remaining >= 0 && timer_running) {
        int rh = remaining / 3600;
        int rm = (remaining % 3600) / 60;
        int rs = remaining % 60;

        printf("\r  \033[1;%sm%02d:%02d:%02d\033[0m  ",
               remaining <= 10 ? "31" : remaining <= 60 ? "33" : "36",
               rh, rm, rs);
        draw_progress(remaining, total);
        fflush(stdout);

        if (remaining == 0) break;
        sleep(1);
        remaining--;
    }

    printf("\n\n");
    printf("  ╔══════════════════════════╗\n");
    printf("  ║  \033[1;32m⏰  TIMER FINISHED!  \033[0m    ║\n");
    printf("  ╚══════════════════════════╝\n");
    printf("\a\a\a");  /* terminal bell */
    fflush(stdout);
    LOG_INFO("Timer: %d seconds completed", total);

    printf("\n  Press Enter to close...");
    fflush(stdout);
    getchar();

    TASK_END();
    return 0;
}
