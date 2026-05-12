#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

static const char *sentences[] = {
    "The quick brown fox jumps over the lazy dog",
    "A fast typist can type more than one hundred words per minute",
    "LunuxOS is a powerful operating system simulator written in C",
    "Multitasking allows multiple processes to run simultaneously",
    "Deadlock detection prevents system resources from being blocked forever",
    "The scheduler uses round robin FCFS and priority queuing strategies",
    "Synchronization primitives include semaphores mutexes and condition variables",
    "Fork and exec are fundamental system calls for process creation in Linux",
};
#define NUM_SENTENCES (int)(sizeof(sentences)/sizeof(sentences[0]))

static int count_correct(const char *original, const char *typed) {
    int correct = 0;
    int len = (int)strlen(original);
    for (int i = 0; i < len && typed[i]; i++)
        if (original[i] == typed[i]) correct++;
    return correct;
}

static int count_words(const char *s) {
    int words = 0, in_word = 0;
    while (*s) {
        if (*s == ' ') in_word = 0;
        else if (!in_word) { words++; in_word = 1; }
        s++;
    }
    return words;
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("TypingTest", RAM_TYPING, 0, 6, QUEUE_RR);

    printf("\033[2J\033[H");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║       LunuxOS — Typing Speed Tester          ║\n");
    printf("╠═══════════════════════════════════════════════╣\n");
    printf("║  Tests: 5 rounds  |  Scoring: WPM + accuracy ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");

    srand((unsigned)time(NULL));
    int rounds    = 5;
    double total_wpm = 0.0;
    double total_acc = 0.0;

    for (int r = 0; r < rounds; r++) {
        int idx = rand() % NUM_SENTENCES;
        const char *target = sentences[idx];

        printf("  ── Round %d / %d ────────────────────────────\n", r + 1, rounds);
        printf("  Type this sentence:\n\n");
        printf("  \033[1;36m%s\033[0m\n\n", target);
        printf("  Press Enter when ready, then type immediately.\n");
        fflush(stdout);
        getchar(); /* wait */

        printf("  GO> ");
        fflush(stdout);

        struct timespec t_start, t_end;
        clock_gettime(CLOCK_MONOTONIC, &t_start);

        char typed[512];
        if (!fgets(typed, sizeof(typed), stdin)) break;
        typed[strcspn(typed, "\n")] = '\0';

        clock_gettime(CLOCK_MONOTONIC, &t_end);

        double elapsed_sec = (t_end.tv_sec  - t_start.tv_sec) +
                             (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
        if (elapsed_sec < 0.1) elapsed_sec = 0.1;

        int    words   = count_words(typed);
        double wpm     = (words / elapsed_sec) * 60.0;
        int    correct = count_correct(target, typed);
        int    total_c = (int)strlen(target);
        double acc     = total_c > 0 ? (correct * 100.0) / total_c : 0.0;

        total_wpm += wpm;
        total_acc += acc;

        printf("\n  Time    : %.2f seconds\n", elapsed_sec);
        printf("  WPM     : \033[1;33m%.1f\033[0m\n", wpm);
        printf("  Accuracy: \033[1;%sm%.1f%%\033[0m\n",
               acc >= 90 ? "32" : acc >= 70 ? "33" : "31", acc);

        /* Show diff */
        printf("  Diff    : ");
        int tlen = (int)strlen(target);
        for (int i = 0; i < tlen; i++) {
            if (!typed[i])            printf("\033[31m_\033[0m");
            else if (typed[i] == target[i]) printf("\033[32m%c\033[0m", target[i]);
            else                       printf("\033[31m%c\033[0m", typed[i]);
        }
        printf("\n\n");

        if (r < rounds - 1) {
            printf("  Press Enter for next round...");
            fflush(stdout);
            getchar();
        }
    }

    double avg_wpm = total_wpm / rounds;
    double avg_acc = total_acc / rounds;
    printf("  ══════════════════════════════\n");
    printf("  FINAL RESULTS\n");
    printf("  Average WPM     : \033[1;33m%.1f\033[0m\n", avg_wpm);
    printf("  Average Accuracy: \033[1;%sm%.1f%%\033[0m\n",
           avg_acc >= 90 ? "32" : avg_acc >= 70 ? "33" : "31", avg_acc);
    const char *grade =
        avg_wpm >= 80 && avg_acc >= 95 ? "Expert" :
        avg_wpm >= 60 && avg_acc >= 90 ? "Advanced" :
        avg_wpm >= 40 && avg_acc >= 80 ? "Intermediate" : "Beginner";
    printf("  Level           : \033[1;36m%s\033[0m\n", grade);
    printf("  ══════════════════════════════\n\n");
    LOG_INFO("TypingTest done: WPM=%.1f ACC=%.1f%%", avg_wpm, avg_acc);

    printf("  Press Enter to close...");
    fflush(stdout);
    getchar();
    TASK_END();
    return 0;
}
