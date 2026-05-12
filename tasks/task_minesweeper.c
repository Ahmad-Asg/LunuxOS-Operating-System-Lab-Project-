#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

#define ROWS  9
#define COLS  9
#define MINES 10

typedef struct {
    int mine;
    int revealed;
    int flagged;
    int adj;
} Cell;

static Cell board[ROWS][COLS];
static int  first_move = 1;
static int  cells_revealed = 0;
static int  game_over = 0;

static void place_mines(int safe_r, int safe_c) {
    srand((unsigned)time(NULL));
    int placed = 0;
    while (placed < MINES) {
        int r = rand() % ROWS;
        int c = rand() % COLS;
        if (!board[r][c].mine && !(r==safe_r && c==safe_c)) {
            board[r][c].mine = 1;
            placed++;
        }
    }
    /* Calculate adjacency */
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (board[r][c].mine) continue;
            int cnt = 0;
            for (int dr = -1; dr <= 1; dr++)
                for (int dc = -1; dc <= 1; dc++) {
                    int nr = r+dr, nc = c+dc;
                    if (nr>=0 && nr<ROWS && nc>=0 && nc<COLS && board[nr][nc].mine)
                        cnt++;
                }
            board[r][c].adj = cnt;
        }
    }
}

static void draw_board(int show_all) {
    printf("\033[2J\033[H");
    printf("  LunuxOS — Minesweeper  [Mines: %d | Revealed: %d/%d]\n\n",
           MINES, cells_revealed, ROWS*COLS - MINES);
    printf("     ");
    for (int c = 0; c < COLS; c++) printf("%2d", c+1);
    printf("\n    +");
    for (int c = 0; c < COLS; c++) printf("--");
    printf("+\n");

    for (int r = 0; r < ROWS; r++) {
        printf(" %2d |", r+1);
        for (int c = 0; c < COLS; c++) {
            Cell *cell = &board[r][c];
            if (show_all && cell->mine) { printf("\033[31m💣\033[0m"); continue; }
            if (!cell->revealed) {
                if (cell->flagged) printf("\033[33m🚩\033[0m");
                else               printf(" ▓");
            } else if (cell->mine) {
                printf("\033[31m💣\033[0m");
            } else if (cell->adj == 0) {
                printf("  ");
            } else {
                const char *colors[] = {"","32","34","31","35","36","33","37","90"};
                printf("\033[%sm %d\033[0m", colors[cell->adj], cell->adj);
            }
        }
        printf("|\n");
    }
    printf("    +");
    for (int c = 0; c < COLS; c++) printf("--");
    printf("+\n");
    printf("\n  Commands:  r <row> <col>  = reveal\n");
    printf("             f <row> <col>  = flag/unflag\n");
    printf("             q             = quit\n\n");
}

static void flood_reveal(int r, int c) {
    if (r<0||r>=ROWS||c<0||c>=COLS) return;
    Cell *cell = &board[r][c];
    if (cell->revealed || cell->flagged || cell->mine) return;
    cell->revealed = 1;
    cells_revealed++;
    if (cell->adj == 0) {
        for (int dr=-1; dr<=1; dr++)
            for (int dc=-1; dc<=1; dc++)
                flood_reveal(r+dr, c+dc);
    }
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("Minesweeper", RAM_MINESWEEPER, 0, 5, QUEUE_RR);

    memset(board, 0, sizeof(board));
    char line[64];
    char cmd;
    int r, c;

    draw_board(0);

    while (!game_over) {
        printf("  > ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        if (line[0] == 'q') break;

        if (sscanf(line, " %c %d %d", &cmd, &r, &c) != 3) {
            printf("  Usage: r/f <row> <col>\n"); continue;
        }
        r--; c--;
        if (r<0||r>=ROWS||c<0||c>=COLS) { printf("  Out of bounds.\n"); continue; }

        if (cmd == 'f' || cmd == 'F') {
            if (!board[r][c].revealed)
                board[r][c].flagged = !board[r][c].flagged;
            draw_board(0);
            continue;
        }

        if (cmd != 'r' && cmd != 'R') { printf("  Unknown command.\n"); continue; }

        if (board[r][c].revealed) { printf("  Already revealed.\n"); continue; }
        if (board[r][c].flagged)  { printf("  Unflag first.\n"); continue; }

        if (first_move) {
            place_mines(r, c);
            first_move = 0;
        }

        if (board[r][c].mine) {
            board[r][c].revealed = 1;
            draw_board(1);
            printf("\n  \033[1;31m💥 BOOM! You hit a mine! Game over.\033[0m\n\n");
            game_over = 1;
            break;
        }

        flood_reveal(r, c);

        if (cells_revealed == ROWS * COLS - MINES) {
            draw_board(0);
            printf("\n  \033[1;32m🎉 You win! All safe cells revealed!\033[0m\n\n");
            LOG_INFO("Minesweeper: player won");
            game_over = 1;
            break;
        }

        draw_board(0);
    }

    printf("  Press Enter to close...");
    fflush(stdout);
    getchar();
    TASK_END();
    return 0;
}
