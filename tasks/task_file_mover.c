#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("FileMover", RAM_FILE_OPS, 0, 5, QUEUE_FCFS);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════╗\n");
    printf("║     LunuxOS — File Mover             ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    char src[MAX_PATH_LEN], dst[MAX_PATH_LEN];
    printf("  Source file     : ");
    fflush(stdout);
    if (!fgets(src, sizeof(src), stdin)) { TASK_END(); return 0; }
    src[strcspn(src, "\n")] = '\0';

    printf("  Destination path: ");
    fflush(stdout);
    if (!fgets(dst, sizeof(dst), stdin)) { TASK_END(); return 0; }
    dst[strcspn(dst, "\n")] = '\0';

    if (!strlen(src) || !strlen(dst)) {
        printf("  Error: empty path.\n"); TASK_END(); return 1;
    }

    struct stat st;
    if (stat(src, &st) != 0) {
        printf("  Error: source file '%s' not found.\n", src);
        TASK_END(); return 1;
    }

    /* Try rename first (same filesystem) */
    if (rename(src, dst) == 0) {
        printf("\n  ✓ Moved '%s' → '%s'\n\n", src, dst);
        LOG_INFO("FileMover: '%s' -> '%s'", src, dst);
    } else {
        /* Different filesystem: copy then delete */
        FILE *fsrc = fopen(src, "rb");
        if (!fsrc) { perror("  Open source"); TASK_END(); return 1; }
        FILE *fdst = fopen(dst, "wb");
        if (!fdst) { perror("  Open dest"); fclose(fsrc); TASK_END(); return 1; }
        char buf[4096]; size_t n;
        while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0) fwrite(buf, 1, n, fdst);
        fclose(fsrc); fclose(fdst);
        unlink(src);
        printf("\n  ✓ Moved '%s' → '%s' (cross-device)\n\n", src, dst);
        LOG_INFO("FileMover(cross): '%s' -> '%s'", src, dst);
    }

    printf("  Press Enter to close...");
    fflush(stdout);
    getchar();

    TASK_END();
    return 0;
}
