#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

#define MAX_ARGS 64
#define MAX_HIST 20

static char history[MAX_HIST][256];
static int  hist_count = 0;

static void add_history(const char *cmd) {
    if (hist_count < MAX_HIST) {
        strncpy(history[hist_count], cmd, 255);
        hist_count++;
    } else {
        memmove(history[0], history[1], (MAX_HIST - 1) * 256);
        strncpy(history[MAX_HIST - 1], cmd, 255);
    }
}

static void run_builtin_cd(char **args) {
    const char *target = args[1] ? args[1] : getenv("HOME");
    if (!target) target = "/";
    if (chdir(target) != 0)
        fprintf(stderr, "  cd: %s: %s\n", target, strerror(errno));
}

static void run_builtin_history(void) {
    for (int i = 0; i < hist_count; i++)
        printf("  %3d  %s\n", i + 1, history[i]);
}

static int execute_command(char *line) {
    char *args[MAX_ARGS];
    int   argc = 0;
    char *tok  = strtok(line, " \t");
    while (tok && argc < MAX_ARGS - 1) {
        args[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    args[argc] = NULL;
    if (argc == 0) return 0;

    if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) return -1;
    if (strcmp(args[0], "cd")   == 0) { run_builtin_cd(args); return 0; }
    if (strcmp(args[0], "history") == 0) { run_builtin_history(); return 0; }
    if (strcmp(args[0], "clear")   == 0) { printf("\033[2J\033[H"); return 0; }
    if (strcmp(args[0], "help")    == 0) {
        printf("  Built-ins: cd, exit, quit, history, clear, help\n");
        printf("  Any other command is executed via fork/exec\n");
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 0; }
    if (pid == 0) {
        execvp(args[0], args);
        fprintf(stderr, "  shell: command not found: %s\n", args[0]);
        exit(127);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
            ; /* already printed */
    }
    return 0;
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  SIG_IGN); /* shell ignores Ctrl+C itself */

    TASK_START("MiniShell", RAM_SHELL, 0, 8, QUEUE_RR);

    printf("\033[2J\033[H");
    printf("╔═══════════════════════════════════════════╗\n");
    printf("║       LunuxOS — Mini Shell Terminal       ║\n");
    printf("║  Type 'help' for built-ins, 'exit' to quit║\n");
    printf("╚═══════════════════════════════════════════╝\n\n");

    char line[512];
    char cwd[512];
    while (1) {
        if (!getcwd(cwd, sizeof(cwd))) strcpy(cwd, "?");
        printf("\033[1;32mLunuxOS\033[0m:\033[1;34m%s\033[0m$ ", cwd);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';
        if (!strlen(line)) continue;
        add_history(line);

        char copy[512];
        strncpy(copy, line, 511);
        copy[511] = '\0';
        if (execute_command(copy) == -1) break;
    }

    printf("\n  MiniShell closed.\n");
    TASK_END();
    return 0;
}
