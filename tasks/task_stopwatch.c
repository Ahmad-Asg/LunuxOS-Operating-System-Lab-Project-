#include "../include/task_common.h"

static volatile int sw_running = 1;
static volatile int paused     = 0;
static void cleanup(int sig) { (void)sig; sw_running = 0; TASK_END(); exit(0); }

static void *display_thread(void *arg) {
    long *elapsed_ms = (long *)arg;
    while (sw_running) {
        if (!paused) {
            long ms  = *elapsed_ms;
            int  h   = (int)(ms / 3600000);
            int  m   = (int)((ms % 3600000) / 60000);
            int  s   = (int)((ms % 60000) / 1000);
            int  cs  = (int)((ms % 1000) / 10);
            printf("\r  \033[1;36m%02d:%02d:%02d.%02d\033[0m  ", h, m, s, cs);
            fflush(stdout);
        }
        usleep(10000); /* 10ms */
    }
    return NULL;
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("Stopwatch", RAM_STOPWATCH, 0, 6, QUEUE_RR);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════╗\n");
    printf("║      LunuxOS — Stopwatch             ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║  Enter: start/stop  r: reset  q: quit║\n");
    printf("╚══════════════════════════════════════╝\n\n");
    printf("  Time: 00:00:00.00\n\n");

    struct timespec start_ts, now_ts;
    long elapsed_ms  = 0;
    long saved_ms    = 0;
    int  running_sw  = 0;
    int  lap_count   = 0;

    pthread_t disp_tid;
    pthread_create(&disp_tid, NULL, display_thread, &elapsed_ms);

    char c;
    while (sw_running) {
        c = (char)getchar();
        if (c == 'q' || c == 'Q') break;
        if (c == '\n' || c == ' ') {
            if (!running_sw) {
                clock_gettime(CLOCK_MONOTONIC, &start_ts);
                running_sw = 1; paused = 0;
                printf("\n  [Running...]\n");
            } else {
                clock_gettime(CLOCK_MONOTONIC, &now_ts);
                saved_ms += (now_ts.tv_sec  - start_ts.tv_sec ) * 1000L
                           + (now_ts.tv_nsec - start_ts.tv_nsec) / 1000000L;
                elapsed_ms = saved_ms;
                running_sw = 0; paused = 1;
                printf("\n  [Paused]\n");
            }
        } else if (c == 'r' || c == 'R') {
            running_sw = 0; paused = 1;
            elapsed_ms = 0; saved_ms = 0;
            printf("\n  [Reset]\n");
        } else if (c == 'l' || c == 'L') {
            lap_count++;
            printf("\n  Lap %d: %ldms\n", lap_count, elapsed_ms);
        }

        if (running_sw) {
            clock_gettime(CLOCK_MONOTONIC, &now_ts);
            elapsed_ms = saved_ms +
                         (now_ts.tv_sec  - start_ts.tv_sec ) * 1000L +
                         (now_ts.tv_nsec - start_ts.tv_nsec) / 1000000L;
        }
    }

    sw_running = 0;
    pthread_join(disp_tid, NULL);
    printf("\n\n  Stopwatch closed. Final: %ldms\n", elapsed_ms);
    TASK_END();
    return 0;
}
