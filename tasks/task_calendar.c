#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

static int days_in_month(int m, int y) {
    int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && ((y%4==0 && y%100!=0) || y%400==0)) return 29;
    return days[m-1];
}

/* Returns 0=Sun..6=Sat for the 1st of month m, year y (Tomohiko Sakamoto) */
static int first_day(int y, int m) {
    static int t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
    if (m < 3) y--;
    return (y + y/4 - y/100 + y/400 + t[m-1] + 1) % 7;
}

static void print_calendar(int m, int y) {
    const char *months[] = {"","January","February","March","April","May","June",
                             "July","August","September","October","November","December"};
    int today_day = -1;
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    if (lt->tm_mon + 1 == m && lt->tm_year + 1900 == y)
        today_day = lt->tm_mday;

    printf("\n  ╔═══════════════════════════════╗\n");
    printf("  ║   %-10s  %4d           ║\n", months[m], y);
    printf("  ╠═══════════════════════════════╣\n");
    printf("  ║  Su Mo Tu We Th Fr Sa         ║\n");
    printf("  ║");

    int fd = first_day(y, m);
    int dm = days_in_month(m, y);
    int col = 0;

    /* leading spaces */
    for (int i = 0; i < fd; i++) { printf("   "); col++; }
    for (int d = 1; d <= dm; d++) {
        if (d == today_day) printf("\033[1;33m %2d\033[0m", d);
        else                printf(" %2d", d);
        col++;
        if (col % 7 == 0 && d < dm) { printf("  ║\n  ║"); }
    }
    /* trailing spaces to fill last row */
    while (col % 7 != 0) { printf("   "); col++; }
    printf("  ║\n");
    printf("  ╚═══════════════════════════════╝\n");
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("Calendar", RAM_CALENDAR, 0, 4, QUEUE_FCFS);

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    int cur_m = lt->tm_mon + 1;
    int cur_y = lt->tm_year + 1900;

    printf("\033[2J\033[H");
    printf("  LunuxOS — Calendar\n");
    printf("  Commands: n=next month  p=prev  t=today  q=quit\n");

    int m = cur_m, y = cur_y;
    while (1) {
        print_calendar(m, y);
        printf("\n  calendar> ");
        fflush(stdout);
        char c = (char)getchar();
        if (c == '\n') c = (char)getchar();
        if (c == 'q' || c == 'Q') break;
        if (c == 'n') { m++; if (m > 12) { m = 1; y++; } }
        if (c == 'p') { m--; if (m < 1)  { m = 12; y--; } }
        if (c == 't') { m = cur_m; y = cur_y; }
    }

    printf("\n  Calendar closed.\n");
    TASK_END();
    return 0;
}
