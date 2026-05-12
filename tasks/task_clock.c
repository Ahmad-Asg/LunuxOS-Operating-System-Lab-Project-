#include "../include/task_common.h"

static volatile int clock_running = 1;
static void cleanup(int sig) { (void)sig; clock_running = 0; TASK_END(); exit(0); }

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("Clock", RAM_CLOCK, 0, 5, QUEUE_PRIO);

    printf("\033[2J");
    printf("\033[?25l"); /* hide cursor */

    while (clock_running) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char date_str[64], time_str[64], day_str[32];
        strftime(date_str, sizeof(date_str), "%B %d, %Y", t);
        strftime(time_str, sizeof(time_str), "%H:%M:%S", t);
        strftime(day_str,  sizeof(day_str),  "%A", t);

        printf("\033[H");
        printf("\033[36m"); /* cyan */
        printf("\n");
        printf("  ╔═══════════════════════════════════╗\n");
        printf("  ║         LunuxOS — Clock           ║\n");
        printf("  ╠═══════════════════════════════════╣\n");
        printf("  ║                                   ║\n");
        printf("  ║   \033[1;33m%s\033[0m\033[36m              ║\n", day_str);
        printf("  ║   \033[1;37m%-27s\033[0m\033[36m  ║\n", date_str);
        printf("  ║                                   ║\n");
        printf("  ║   \033[1;32m%s\033[0m\033[36m                  ║\n", time_str);
        printf("  ║                                   ║\n");
        printf("  ╚═══════════════════════════════════╝\n");
        printf("  \033[0m  Press Ctrl+C to close\n");
        fflush(stdout);
        sleep(1);
    }

    printf("\033[?25h"); /* show cursor */
    TASK_END();
    return 0;
}
