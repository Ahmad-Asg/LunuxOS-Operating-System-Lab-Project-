#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

typedef struct { char board[9]; char cur; int vs_cpu; } TTT;

static void draw_board(TTT *g) {
    printf("\033[2J\033[H");
    printf("  LunuxOS — Tic Tac Toe\n\n");
    printf("  Positions:    Board:\n");
    printf("   1 │ 2 │ 3    %c │ %c │ %c\n",
           g->board[0] ? g->board[0] : '.',
           g->board[1] ? g->board[1] : '.',
           g->board[2] ? g->board[2] : '.');
    printf("  ───┼───┼───   ───┼───┼───\n");
    printf("   4 │ 5 │ 6    %c │ %c │ %c\n",
           g->board[3] ? g->board[3] : '.',
           g->board[4] ? g->board[4] : '.',
           g->board[5] ? g->board[5] : '.');
    printf("  ───┼───┼───   ───┼───┼───\n");
    printf("   7 │ 8 │ 9    %c │ %c │ %c\n",
           g->board[6] ? g->board[6] : '.',
           g->board[7] ? g->board[7] : '.',
           g->board[8] ? g->board[8] : '.');
    printf("\n  Current player: \033[1;%sm%c\033[0m\n",
           g->cur == 'X' ? "31" : "34", g->cur);
}

static int check_win(TTT *g, char p) {
    int w[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    for (int i = 0; i < 8; i++)
        if (g->board[w[i][0]] == p && g->board[w[i][1]] == p && g->board[w[i][2]] == p)
            return 1;
    return 0;
}

static int board_full(TTT *g) {
    for (int i = 0; i < 9; i++) if (!g->board[i]) return 0;
    return 1;
}

/* Simple CPU: take winning move, block, else center, else random */
static int cpu_move(TTT *g) {
    int w[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    /* Try to win */
    for (int i = 0; i < 8; i++) {
        int a=w[i][0], b=w[i][1], c=w[i][2];
        if (g->board[a]=='O' && g->board[b]=='O' && !g->board[c]) return c;
        if (g->board[a]=='O' && g->board[c]=='O' && !g->board[b]) return b;
        if (g->board[b]=='O' && g->board[c]=='O' && !g->board[a]) return a;
    }
    /* Block X win */
    for (int i = 0; i < 8; i++) {
        int a=w[i][0], b=w[i][1], c=w[i][2];
        if (g->board[a]=='X' && g->board[b]=='X' && !g->board[c]) return c;
        if (g->board[a]=='X' && g->board[c]=='X' && !g->board[b]) return b;
        if (g->board[b]=='X' && g->board[c]=='X' && !g->board[a]) return a;
    }
    /* Center */
    if (!g->board[4]) return 4;
    /* Corners */
    int corners[] = {0,2,6,8};
    for (int i = 0; i < 4; i++) if (!g->board[corners[i]]) return corners[i];
    /* Any free */
    for (int i = 0; i < 9; i++) if (!g->board[i]) return i;
    return -1;
}

static void play_game(TTT *g) {
    memset(g->board, 0, 9);
    g->cur = 'X';
    char buf[16];

    while (1) {
        draw_board(g);

        int pos;
        if (g->vs_cpu && g->cur == 'O') {
            pos = cpu_move(g);
            printf("  CPU chose position %d\n", pos + 1);
            usleep(500000);
        } else {
            printf("  Enter position (1-9): ");
            fflush(stdout);
            if (!fgets(buf, sizeof(buf), stdin)) return;
            pos = atoi(buf) - 1;
        }

        if (pos < 0 || pos > 8 || g->board[pos]) {
            printf("  Invalid move!\n"); sleep(1); continue;
        }
        g->board[pos] = g->cur;

        if (check_win(g, g->cur)) {
            draw_board(g);
            printf("\n  \033[1;32m🎉 Player %c wins!\033[0m\n\n", g->cur);
            LOG_INFO("TicTacToe: %c won", g->cur);
            return;
        }
        if (board_full(g)) {
            draw_board(g);
            printf("\n  \033[33mDraw! No winner.\033[0m\n\n");
            return;
        }
        g->cur = (g->cur == 'X') ? 'O' : 'X';
    }
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("TicTacToe", RAM_TICTACTOE, 0, 5, QUEUE_RR);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════╗\n");
    printf("║  LunuxOS — Tic Tac Toe      ║\n");
    printf("╠══════════════════════════════╣\n");
    printf("║  1. 2 Players               ║\n");
    printf("║  2. vs Computer (AI)        ║\n");
    printf("╚══════════════════════════════╝\n\n");

    TTT game;
    char buf[16];

    while (1) {
        printf("  Choose mode (1/2) or 'q' quit: ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (buf[0] == 'q') break;
        game.vs_cpu = (buf[0] == '2');
        play_game(&game);
        printf("  Play again? (y/n): ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (buf[0] != 'y' && buf[0] != 'Y') break;
    }

    printf("  Goodbye!\n");
    TASK_END();
    return 0;
}
