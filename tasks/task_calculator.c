#include "../include/task_common.h"

static void cleanup(int sig) { (void)sig; TASK_END(); exit(0); }

int main(void) {
    signal(SIGTERM, cleanup);
    signal(SIGINT,  cleanup);

    TASK_START("Calculator", RAM_CALCULATOR, 0, 7, QUEUE_RR);

    printf("\033[2J\033[H");
    printf("╔══════════════════════════════════════╗\n");
    printf("║       LunuxOS — Calculator           ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║  Enter expression: e.g.  12.5 + 3   ║\n");
    printf("║  Operators: + - * /  |  'q' to quit ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    char line[256];
    while (1) {
        printf("  calc> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == 'q' || line[0] == 'Q') break;

        double a, b;
        char   op;
        if (sscanf(line, "%lf %c %lf", &a, &op, &b) != 3) {
            printf("  Error: invalid expression. Try:  3 + 5\n");
            continue;
        }
        double result;
        int    ok = 1;
        switch (op) {
            case '+': result = a + b; break;
            case '-': result = a - b; break;
            case '*': result = a * b; break;
            case '/':
                if (b == 0.0) { printf("  Error: division by zero\n"); ok = 0; }
                else result = a / b;
                break;
            default:
                printf("  Error: unknown operator '%c'\n", op);
                ok = 0;
        }
        if (ok) printf("  Result: %g %c %g = %g\n\n", a, op, b, result);
    }

    printf("\n  Calculator closed.\n");
    TASK_END();
    return 0;
}
