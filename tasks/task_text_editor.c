#include "../include/task_common.h"

#define MAX_LINES 500
#define LINE_LEN  512

static char  lines[MAX_LINES][LINE_LEN];
static int   total_lines = 0;
static char  filename[MAX_PATH_LEN] = "";
static int   modified = 0;

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

static void display_buffer(int start, int count) {
    printf("\033[2J\033[H");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║  LunuxOS Text Editor  |  %-24s  ║\n", strlen(filename) ? filename : "[no file]");
    printf("║  Lines: %-4d  |  :w=save  :q=quit  :h=help       ║\n", total_lines);
    printf("╠═══════════════════════════════════════════════════╣\n");
    int end = start + count;
    if (end > total_lines) end = total_lines;
    for (int i = start; i < end; i++)
        printf("  \033[33m%3d\033[0m │ %s\n", i + 1, lines[i]);
    printf("╚═══════════════════════════════════════════════════╝\n");
    if (modified) printf("  [modified]\n");
}

static void save_file(void) {
    if (!strlen(filename)) {
        printf("  Enter filename to save: ");
        fflush(stdout);
        if (!fgets(filename, sizeof(filename), stdin)) return;
        filename[strcspn(filename, "\n")] = '\0';
    }
    FILE *f = fopen(filename, "w");
    if (!f) { perror("  Cannot save"); return; }
    for (int i = 0; i < total_lines; i++) fprintf(f, "%s\n", lines[i]);
    fclose(f);
    modified = 0;
    printf("  Saved %d lines to '%s'.\n", total_lines, filename);
    LOG_INFO("TextEditor: saved '%s'", filename);
}

static void load_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    total_lines = 0;
    while (total_lines < MAX_LINES && fgets(lines[total_lines], LINE_LEN, f)) {
        lines[total_lines][strcspn(lines[total_lines], "\n")] = '\0';
        total_lines++;
    }
    fclose(f);
}

static void insert_line(int after, const char *text) {
    if (total_lines >= MAX_LINES) { printf("  Buffer full.\n"); return; }
    memmove(lines[after + 1], lines[after], (size_t)(total_lines - after) * LINE_LEN);
    strncpy(lines[after], text, LINE_LEN - 1);
    total_lines++;
    modified = 1;
}

static void delete_line(int idx) {
    if (idx < 0 || idx >= total_lines) { printf("  Line %d out of range.\n", idx + 1); return; }
    memmove(lines[idx], lines[idx + 1], (size_t)(total_lines - idx - 1) * LINE_LEN);
    total_lines--;
    modified = 1;
}

static void search(const char *term) {
    int found = 0;
    for (int i = 0; i < total_lines; i++) {
        if (strstr(lines[i], term)) {
            printf("  Line %3d: %s\n", i + 1, lines[i]);
            found++;
        }
    }
    if (!found) printf("  Not found: '%s'\n", term);
}

int main(int argc, char *argv[]) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("TextEditor", RAM_EDITOR, 0, 7, QUEUE_RR);

    if (argc > 1) {
        strncpy(filename, argv[1], MAX_PATH_LEN - 1);
        load_file(filename);
    }

    char cmd[LINE_LEN];
    int  view_start = 0;

    display_buffer(view_start, 20);

    while (1) {
        printf("\n  cmd> ");
        fflush(stdout);
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        cmd[strcspn(cmd, "\n")] = '\0';

        if (cmd[0] == ':') {
            if (strcmp(cmd, ":q") == 0) {
                if (modified) {
                    printf("  Unsaved changes. :q! to force quit, :w to save: ");
                    fflush(stdout);
                    char ans[16]; fgets(ans, sizeof(ans), stdin);
                    if (strncmp(ans, ":q!", 3) != 0) continue;
                }
                break;
            }
            if (strcmp(cmd, ":q!") == 0)  break;
            if (strcmp(cmd, ":w")  == 0)  { save_file(); continue; }
            if (strncmp(cmd, ":w ", 3) == 0) {
                strncpy(filename, cmd + 3, MAX_PATH_LEN - 1);
                save_file(); continue;
            }
            if (strncmp(cmd, ":i ", 3) == 0) {
                /* :i <after_line> text */
                int ln; char text[LINE_LEN];
                if (sscanf(cmd + 3, "%d %[^\n]", &ln, text) >= 1) {
                    if (ln < 0 || ln > total_lines) ln = total_lines;
                    char *t = (ln < (int)strlen(cmd+3)) ? text : "";
                    insert_line(ln, t);
                    printf("  Inserted line after %d.\n", ln);
                }
                display_buffer(view_start, 20); continue;
            }
            if (strncmp(cmd, ":d ", 3) == 0) {
                int ln = atoi(cmd + 3) - 1;
                delete_line(ln);
                printf("  Deleted line %d.\n", ln + 1);
                display_buffer(view_start, 20); continue;
            }
            if (strncmp(cmd, ":s ", 3) == 0) {
                search(cmd + 3); continue;
            }
            if (strncmp(cmd, ":g ", 3) == 0) {
                view_start = atoi(cmd + 3) - 1;
                if (view_start < 0) view_start = 0;
                display_buffer(view_start, 20); continue;
            }
            if (strcmp(cmd, ":h") == 0) {
                printf("  :w [file]  save          :q   quit\n");
                printf("  :i <n> txt insert after line n\n");
                printf("  :d <n>     delete line n\n");
                printf("  :s <term>  search\n");
                printf("  :g <n>     go to line n\n");
                printf("  Just type and Enter to append a line\n");
                continue;
            }
            printf("  Unknown command: %s\n", cmd);
        } else if (strlen(cmd) > 0) {
            /* Append line */
            if (total_lines < MAX_LINES) {
                strncpy(lines[total_lines], cmd, LINE_LEN - 1);
                total_lines++;
                modified = 1;
                view_start = total_lines > 20 ? total_lines - 20 : 0;
                display_buffer(view_start, 20);
            } else {
                printf("  Buffer full.\n");
            }
        }
    }

    printf("\n  Text Editor closed.\n");
    TASK_END();
    return 0;
}
