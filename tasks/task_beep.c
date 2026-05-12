#include "../include/task_common.h"

static volatile int beep_running = 1;
static void cleanup(int sig) { (void)sig; beep_running = 0; TASK_END(); exit(0); }

/* Simple melody using terminal bell sequences + visual display */
typedef struct { int note_ms; int rest_ms; const char *name; } Note;

static const Note melody[] = {
    {300, 100, "C4"}, {300, 100, "E4"}, {300, 100, "G4"}, {600, 200, "C5"},
    {300, 100, "G4"}, {300, 100, "E4"}, {600, 200, "C4"},
    {300, 100, "F4"}, {300, 100, "A4"}, {300, 100, "C5"}, {600, 200, "F5"},
    {300, 100, "C5"}, {300, 100, "A4"}, {600, 200, "F4"},
    {300, 100, "G4"}, {300, 100, "B4"}, {300, 100, "D5"}, {600, 200, "G5"},
    {300, 100, "D5"}, {300, 100, "B4"}, {600, 300, "G4"},
};
#define MELODY_LEN  (int)(sizeof(melody)/sizeof(melody[0]))

/* Visual piano bar */
static const char *piano_keys = "C D EF G A BC D EF G A B";

static void draw_piano(int active) {
    const Note *n = &melody[active % MELODY_LEN];
    printf("\r  ♪ Playing: \033[1;33m%-4s\033[0m  ", n->name);
    printf("[");
    for (int i = 0; i < (int)strlen(piano_keys); i++) {
        if (piano_keys[i] == n->name[0] && i % 2 == 0)
            printf("\033[1;32m█\033[0m");
        else
            printf("%s", piano_keys[i] == ' ' ? " " : "\xe2\x96\x93");
    }
    printf("]  ");
    fflush(stdout);
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("BeepMusic", RAM_BEEP, 0, 2, QUEUE_PRIO);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════════╗\n");
    printf("║    LunuxOS — Background Beep / Music     ║\n");
    printf("║    Press Ctrl+C to stop                  ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");
    printf("  Visual player (terminal bell + display):\n\n");

    int idx = 0;
    int repeats = 0;

    while (beep_running && repeats < 3) {
        const Note *n = &melody[idx];
        draw_piano(idx);

        /* Terminal bell for audio */
        printf("\a");
        fflush(stdout);

        /* Simulate note duration */
        int total_ms = n->note_ms + n->rest_ms;
        int slices   = total_ms / 50;
        for (int s = 0; s < slices && beep_running; s++) {
            usleep(50000);
            /* Animate bar */
            int bar = (int)(((double)(n->note_ms) / 600.0) * 20);
            printf("\r  ♪ Playing: \033[1;33m%-4s\033[0m  Volume: [", n->name);
            for (int b = 0; b < 20; b++)
                printf("%s", b < bar ? "\033[1;32m█\033[0m" : "░");
            printf("] ");
            fflush(stdout);
        }

        idx++;
        if (idx >= MELODY_LEN) { idx = 0; repeats++; }
    }

    printf("\n\n  BeepMusic finished.\n");
    TASK_END();
    return 0;
}
