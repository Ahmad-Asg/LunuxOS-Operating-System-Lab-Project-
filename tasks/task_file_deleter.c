#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("FileDeleter", RAM_FILE_OPS, 0, 5, QUEUE_FCFS);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════╗\n");
    printf("║     LunuxOS — File Deleter           ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    char path[MAX_PATH_LEN];
    printf("  File to delete: ");
    fflush(stdout);
    if (!fgets(path, sizeof(path), stdin)) { TASK_END(); return 0; }
    path[strcspn(path, "\n")] = '\0';

    if (!strlen(path)) {
        printf("  No path given.\n"); TASK_END(); return 0;
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        printf("  Error: file '%s' not found.\n", path);
        TASK_END(); return 1;
    }

    printf("\n  File         : %s\n", path);
    printf("  Size         : %ld bytes\n", (long)st.st_size);
    printf("\n  Are you sure you want to delete this file? (yes/no): ");
    fflush(stdout);

    char ans[16];
    if (!fgets(ans, sizeof(ans), stdin)) { TASK_END(); return 0; }
    ans[strcspn(ans, "\n")] = '\0';

    if (strcmp(ans, "yes") == 0) {
        if (unlink(path) == 0) {
            printf("\n  ✓ File '%s' deleted successfully.\n\n", path);
            LOG_INFO("FileDeleter: deleted '%s'", path);
        } else {
            perror("  Error deleting file");
        }
    } else {
        printf("  Deletion cancelled.\n");
    }

    printf("  Press Enter to close...");
    fflush(stdout);
    getchar();

    TASK_END();
    return 0;
}
