#include "../include/task_common.h"

static volatile int cpu_mon_running = 1;
static void cleanup(int sig) { (void)sig; cpu_mon_running = 0; TASK_END(); exit(0); }

#define MAX_CPUS 32

typedef struct { long user; long nice; long sys; long idle; long iowait; long irq; long softirq; } CPUStat;

static int read_cpu_stats(CPUStat stats[], int max_cpus) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0;
    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count <= max_cpus) {
        if (strncmp(line, "cpu", 3) != 0) break;
        if (line[3] == ' ') { count = 0; continue; } /* skip total line */
        CPUStat *s = &stats[count];
        sscanf(line, "cpu%*d %ld %ld %ld %ld %ld %ld %ld",
               &s->user, &s->nice, &s->sys, &s->idle, &s->iowait, &s->irq, &s->softirq);
        count++;
        if (count >= max_cpus) break;
    }
    fclose(f);
    return count;
}

static double calc_usage(CPUStat *a, CPUStat *b) {
    long idle_a = a->idle + a->iowait;
    long idle_b = b->idle + b->iowait;
    long total_a = a->user+a->nice+a->sys+a->idle+a->iowait+a->irq+a->softirq;
    long total_b = b->user+b->nice+b->sys+b->idle+b->iowait+b->irq+b->softirq;
    long dtotal = total_b - total_a;
    long didle  = idle_b  - idle_a;
    if (dtotal <= 0) return 0.0;
    return (double)(dtotal - didle) * 100.0 / (double)dtotal;
}

static void draw_cpu_bar(double pct, int width) {
    int filled = (int)((pct / 100.0) * width);
    const char *color = pct >= 90 ? "\033[31m" : pct >= 70 ? "\033[33m" : "\033[32m";
    printf("%s[", color);
    for (int i = 0; i < width; i++)
        printf("%s", i < filled ? "█" : "░");
    printf("] \033[0m%5.1f%%", pct);
}

static void read_load_avg(double *la1, double *la5, double *la15) {
    FILE *f = fopen("/proc/loadavg", "r");
    *la1 = *la5 = *la15 = 0;
    if (!f) return;
    fscanf(f, "%lf %lf %lf", la1, la5, la15);
    fclose(f);
}

static int read_process_count(void) {
    FILE *f = fopen("/proc/loadavg", "r");
    if (!f) return 0;
    int run = 0, total = 0;
    fscanf(f, "%*f %*f %*f %d/%d", &run, &total);
    fclose(f);
    return total;
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("CPUMonitor", RAM_MONITOR, 0, 3, QUEUE_PRIO);

    printf("\033[?25l");

    CPUStat prev[MAX_CPUS], curr[MAX_CPUS];
    int num_cpus = read_cpu_stats(prev, MAX_CPUS);
    sleep(1);

    /* Rolling history */
    double history[60];
    memset(history, 0, sizeof(history));
    int hist_pos = 0;

    while (cpu_mon_running) {
        num_cpus = read_cpu_stats(curr, MAX_CPUS);
        double total_usage = 0.0;

        printf("\033[2J\033[H");
        printf("╔══════════════════════════════════════════════════════╗\n");
        printf("║            LunuxOS — CPU Monitor                    ║\n");
        printf("╠══════════════════════════════════════════════════════╣\n");
        printf("║  Updates every 1 second │ Ctrl+C to close           ║\n");
        printf("╚══════════════════════════════════════════════════════╝\n\n");

        printf("  ── Per-Core Usage ────────────────────────────────────\n");
        for (int i = 0; i < num_cpus; i++) {
            double usage = calc_usage(&prev[i], &curr[i]);
            total_usage += usage;
            printf("  CPU%-2d ", i);
            draw_cpu_bar(usage, 35);
            printf("\n");
            prev[i] = curr[i];
        }

        double avg = num_cpus > 0 ? total_usage / num_cpus : 0.0;
        history[hist_pos % 60] = avg;
        hist_pos++;

        printf("\n  ── Average CPU Usage ─────────────────────────────────\n");
        printf("  Avg   ");
        draw_cpu_bar(avg, 35);
        printf("\n");

        double la1, la5, la15;
        read_load_avg(&la1, &la5, &la15);
        int procs = read_process_count();

        printf("\n  ── System Info ───────────────────────────────────────\n");
        printf("  Load Avg (1m/5m/15m): %.2f / %.2f / %.2f\n", la1, la5, la15);
        printf("  Total Processes     : %d\n", procs);
        printf("  CPU Cores detected  : %d\n\n", num_cpus);

        /* Sparkline history */
        printf("  ── CPU History (last 30 seconds) ─────────────────────\n  ");
        const char *sparks = "▁▂▃▄▅▆▇█";
        for (int i = 0; i < 30; i++) {
            int idx  = (hist_pos - 30 + i + 60) % 60;
            int lv   = (int)(history[idx] / 12.5);
            if (lv > 7) lv = 7;
            /* Print UTF-8 sparkline character */
            unsigned char ch[4];
            unsigned int code = 0x2581 + (unsigned int)lv;
            ch[0] = (unsigned char)(0xE0 | (code >> 12));
            ch[1] = (unsigned char)(0x80 | ((code >> 6) & 0x3F));
            ch[2] = (unsigned char)(0x80 | (code & 0x3F));
            ch[3] = '\0';
            printf("%s", ch);
            (void)sparks;
        }
        printf("\n\n");

        time_t now = time(NULL);
        char tbuf[32];
        strftime(tbuf, sizeof(tbuf), "%H:%M:%S", localtime(&now));
        printf("  Last update: %s\n", tbuf);
        fflush(stdout);
        sleep(1);
    }

    printf("\033[?25h");
    printf("\n  CPU Monitor closed.\n");
    TASK_END();
    return 0;
}
