#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("FileCopier", RAM_FILE_OPS, 0, 5, QUEUE_FCFS);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════╗\n");
    printf("║     LunuxOS — File Copier            ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    char src[MAX_PATH_LEN], dst[MAX_PATH_LEN];

    printf("  Source file     : ");
    fflush(stdout);
    if (!fgets(src, sizeof(src), stdin)) { TASK_END(); return 0; }
    src[strcspn(src, "\n")] = '\0';

    printf("  Destination file: ");
    fflush(stdout);
    if (!fgets(dst, sizeof(dst), stdin)) { TASK_END(); return 0; }
    dst[strcspn(dst, "\n")] = '\0';

    if (!strlen(src) || !strlen(dst)) {
        printf("  Error: empty path.\n");
        TASK_END(); return 1;
    }

    FILE *fsrc = fopen(src, "rb");
    if (!fsrc) { perror("  Cannot open source"); TASK_END(); return 1; }

    FILE *fdst = fopen(dst, "wb");
    if (!fdst) { perror("  Cannot create destination"); fclose(fsrc); TASK_END(); return 1; }

    char   chunk[4096];
    size_t n;
    long   total = 0;
    while ((n = fread(chunk, 1, sizeof(chunk), fsrc)) > 0) {
        fwrite(chunk, 1, n, fdst);
        total += (long)n;
    }
    fclose(fsrc);
    fclose(fdst);

    printf("\n  ✓ Copied '%s' → '%s'\n", src, dst);
    printf("  ✓ Bytes copied: %ld\n\n", total);
    LOG_INFO("FileCopier: '%s' -> '%s' (%ld bytes)", src, dst, total);

    printf("  Press Enter to close...");
    fflush(stdout);
    getchar();

    TASK_END();
    return 0;
}
