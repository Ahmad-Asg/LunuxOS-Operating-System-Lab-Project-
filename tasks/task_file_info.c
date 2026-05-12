#include "../include/task_common.h"
#include <pwd.h>
#include <grp.h>

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

static void print_permissions(mode_t mode) {
    printf("  Permissions  : %c%c%c%c%c%c%c%c%c%c\n",
           S_ISDIR(mode)  ? 'd' : '-',
           (mode & S_IRUSR) ? 'r' : '-',
           (mode & S_IWUSR) ? 'w' : '-',
           (mode & S_IXUSR) ? 'x' : '-',
           (mode & S_IRGRP) ? 'r' : '-',
           (mode & S_IWGRP) ? 'w' : '-',
           (mode & S_IXGRP) ? 'x' : '-',
           (mode & S_IROTH) ? 'r' : '-',
           (mode & S_IWOTH) ? 'w' : '-',
           (mode & S_IXOTH) ? 'x' : '-');
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("FileInfo", RAM_FILE_OPS, 0, 4, QUEUE_FCFS);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════╗\n");
    printf("║     LunuxOS — File Info Checker      ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    char path[MAX_PATH_LEN];
    while (1) {
        printf("  File path (or 'q' to quit): ");
        fflush(stdout);
        if (!fgets(path, sizeof(path), stdin)) break;
        path[strcspn(path, "\n")] = '\0';
        if (path[0] == 'q' || path[0] == 'Q') break;
        if (!strlen(path)) continue;

        struct stat st;
        if (lstat(path, &st) != 0) {
            printf("  Error: cannot stat '%s': %s\n\n", path, strerror(errno));
            continue;
        }

        char tbuf[64];
        struct tm *t;

        printf("\n  ── File Information ──────────────────────\n");
        printf("  Path         : %s\n", path);
        printf("  Type         : %s\n",
               S_ISREG(st.st_mode)  ? "Regular file" :
               S_ISDIR(st.st_mode)  ? "Directory"    :
               S_ISLNK(st.st_mode)  ? "Symbolic link" :
               S_ISFIFO(st.st_mode) ? "FIFO/pipe"    : "Other");
        printf("  Size         : %ld bytes  (%.2f KB)\n",
               (long)st.st_size, (double)st.st_size / 1024.0);
        printf("  Inode        : %lu\n", (unsigned long)st.st_ino);
        printf("  Hard links   : %lu\n", (unsigned long)st.st_nlink);
        print_permissions(st.st_mode);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        printf("  Owner        : %s (uid=%d)\n",
               pw ? pw->pw_name : "unknown", (int)st.st_uid);
        printf("  Group        : %s (gid=%d)\n",
               gr ? gr->gr_name : "unknown", (int)st.st_gid);

        t = localtime(&st.st_atime);
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", t);
        printf("  Last access  : %s\n", tbuf);

        t = localtime(&st.st_mtime);
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", t);
        printf("  Last modified: %s\n", tbuf);

        t = localtime(&st.st_ctime);
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", t);
        printf("  Status change: %s\n", tbuf);
        printf("  ─────────────────────────────────────────\n\n");
    }

    printf("  FileInfo closed.\n");
    TASK_END();
    return 0;
}
