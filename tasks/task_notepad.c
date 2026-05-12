#include "../include/task_common.h"
#include <pthread.h>

#define SAVE_FILE "/tmp/lunuxos_notepad.txt"
#define MAX_LINES 200
#define LINE_LEN  256

static char    notes[MAX_LINES][LINE_LEN];
static int     line_count = 0;
static pthread_mutex_t notes_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int notepad_running = 1;

static void save_to_disk(void) {
    pthread_mutex_lock(&notes_lock);
    FILE *f = fopen(SAVE_FILE, "w");
    if (f) {
        for (int i = 0; i < line_count; i++)
            fprintf(f, "%s\n", notes[i]);
        fclose(f);
        printf("\n  [Auto-saved to %s]\n  notepad> ", SAVE_FILE);
        fflush(stdout);
    }
    pthread_mutex_unlock(&notes_lock);
}

static void *autosave_thread(void *arg) {
    (void)arg;
    while (notepad_running) {
        sleep(30);
        if (notepad_running) save_to_disk();
    }
    return NULL;
}

static void cleanup(int sig) {
    (void)sig;
    notepad_running = 0;
    save_to_disk();
    TASK_END();
    exit(0);
}

static void load_from_disk(void) {
    FILE *f = fopen(SAVE_FILE, "r");
    if (!f) return;
    char buf[LINE_LEN];
    while (line_count < MAX_LINES && fgets(buf, sizeof(buf), f)) {
        buf[strcspn(buf, "\n")] = '\0';
        strncpy(notes[line_count], buf, LINE_LEN - 1);
        line_count++;
    }
    fclose(f);
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("Notepad", RAM_NOTEPAD, 0, 6, QUEUE_RR);

    load_from_disk();

    pthread_t tid;
    pthread_create(&tid, NULL, autosave_thread, NULL);
    pthread_detach(tid);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════════╗\n");
    printf("║      LunuxOS — Notepad (Auto-Save)       ║\n");
    printf("╠══════════════════════════════════════════╣\n");
    printf("║  Commands:                               ║\n");
    printf("║    :w  – save now    :q  – quit          ║\n");
    printf("║    :l  – list lines  :c  – clear all     ║\n");
    printf("║  Just type and press Enter to add a line ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    if (line_count > 0) {
        printf("  [Loaded %d lines from previous session]\n", line_count);
        for (int i = 0; i < line_count; i++)
            printf("  %3d: %s\n", i + 1, notes[i]);
    }
    printf("\n");

    char buf[LINE_LEN];
    while (1) {
        printf("  notepad> ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) break;
        buf[strcspn(buf, "\n")] = '\0';

        if (strcmp(buf, ":q") == 0) break;
        if (strcmp(buf, ":w") == 0) { save_to_disk(); printf("  Saved.\n"); continue; }
        if (strcmp(buf, ":c") == 0) {
            pthread_mutex_lock(&notes_lock);
            line_count = 0;
            pthread_mutex_unlock(&notes_lock);
            printf("  Cleared all lines.\n");
            continue;
        }
        if (strcmp(buf, ":l") == 0) {
            pthread_mutex_lock(&notes_lock);
            if (!line_count) printf("  (empty)\n");
            for (int i = 0; i < line_count; i++)
                printf("  %3d: %s\n", i + 1, notes[i]);
            pthread_mutex_unlock(&notes_lock);
            continue;
        }
        pthread_mutex_lock(&notes_lock);
        if (line_count < MAX_LINES) {
            strncpy(notes[line_count], buf, LINE_LEN - 1);
            line_count++;
            printf("  Line %d added.\n", line_count);
        } else {
            printf("  Notepad full (%d lines max).\n", MAX_LINES);
        }
        pthread_mutex_unlock(&notes_lock);
    }

    notepad_running = 0;
    save_to_disk();
    printf("\n  Notepad closed. File saved to %s\n", SAVE_FILE);
    TASK_END();
    return 0;
}
