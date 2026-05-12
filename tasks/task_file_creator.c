#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("FileCreator", RAM_FILE_OPS, 0, 5, QUEUE_FCFS);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════╗\n");
    printf("║     LunuxOS — File Creator           ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    char filename[MAX_PATH_LEN];
    printf("  Enter filename to create: ");
    fflush(stdout);
    if (!fgets(filename, sizeof(filename), stdin)) { TASK_END(); return 0; }
    filename[strcspn(filename, "\n")] = '\0';
    if (strlen(filename) == 0) {
        printf("  No filename given. Exiting.\n");
        TASK_END(); return 0;
    }

    /* Check if file exists */
    struct stat st;
    if (stat(filename, &st) == 0) {
        printf("  File '%s' already exists. Overwrite? (y/n): ", filename);
        fflush(stdout);
        char ans[8];
        if (!fgets(ans, sizeof(ans), stdin)) { TASK_END(); return 0; }
        if (ans[0] != 'y' && ans[0] != 'Y') {
            printf("  Cancelled.\n");
            TASK_END(); return 0;
        }
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("  Error creating file");
        TASK_END(); return 1;
    }

    printf("  Enter content (type ':done' on a new line to finish):\n\n");
    char line[512];
    int  lines = 0;
    while (1) {
        printf("  > ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        if (strncmp(line, ":done", 5) == 0) break;
        fprintf(f, "%s", line);
        lines++;
    }
    fclose(f);

    stat(filename, &st);
    printf("\n  ✓ File '%s' created successfully.\n", filename);
    printf("  ✓ Lines written : %d\n", lines);
    printf("  ✓ File size     : %ld bytes\n\n", (long)st.st_size);
    LOG_INFO("FileCreator: created '%s' (%d lines)", filename, lines);

    printf("  Press Enter to close...");
    fflush(stdout);
    getchar();

    TASK_END();
    return 0;
}
