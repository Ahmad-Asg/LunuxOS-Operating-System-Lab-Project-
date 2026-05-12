#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

static void play_round(int max_num, int max_tries) {
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    int secret = rand() % max_num + 1;
    int tries  = 0;
    int won    = 0;

    printf("  Guess a number between 1 and %d (%d tries max)\n\n", max_num, max_tries);

    while (tries < max_tries) {
        printf("  Attempt %d/%d > ", tries + 1, max_tries);
        fflush(stdout);
        char buf[32];
        if (!fgets(buf, sizeof(buf), stdin)) return;
        int guess = atoi(buf);
        tries++;

        if (guess == secret) {
            printf("\n  \033[1;32m✓ Correct! You got it in %d %s!\033[0m\n",
                   tries, tries == 1 ? "try" : "tries");
            won = 1; break;
        } else if (guess < secret) {
            printf("  \033[33m↑ Too low!\033[0m  ");
        } else {
            printf("  \033[33m↓ Too high!\033[0m ");
        }

        int remaining = max_tries - tries;
        if (remaining > 0)
            printf("(%d %s remaining)\n", remaining, remaining == 1 ? "try" : "tries");
    }

    if (!won) {
        printf("\n  \033[31m✗ Game over! The number was \033[1m%d\033[0m\033[31m.\033[0m\n", secret);
    }

    /* Score */
    if (won) {
        int score = (max_tries - tries + 1) * 100 / max_tries;
        printf("  Score: %d / 100\n\n", score);
    }
}

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("NumberGuess", RAM_GUESSING, 0, 5, QUEUE_RR);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════╗\n");
    printf("║   LunuxOS — Number Guessing Game     ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║  1. Easy   (1-50,   10 tries)        ║\n");
    printf("║  2. Medium (1-100,  7 tries)         ║\n");
    printf("║  3. Hard   (1-500,  5 tries)         ║\n");
    printf("║  4. Insane (1-1000, 3 tries)         ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    while (1) {
        printf("  Choose difficulty (1-4) or 'q' to quit: ");
        fflush(stdout);
        char buf[16];
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (buf[0] == 'q' || buf[0] == 'Q') break;
        int choice = atoi(buf);

        int max_num, max_tries;
        switch (choice) {
            case 1: max_num =   50; max_tries = 10; break;
            case 2: max_num =  100; max_tries =  7; break;
            case 3: max_num =  500; max_tries =  5; break;
            case 4: max_num = 1000; max_tries =  3; break;
            default:
                printf("  Invalid choice.\n\n");
                continue;
        }

        play_round(max_num, max_tries);

        printf("  Play again? (y/n): ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (buf[0] != 'y' && buf[0] != 'Y') break;
        printf("\n");
    }

    printf("\n  Thanks for playing! Goodbye.\n");
    TASK_END();
    return 0;
}
