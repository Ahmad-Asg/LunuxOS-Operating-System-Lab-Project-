#include "ipc.h"
#include "logger.h"
#include <sys/msg.h>

/* ══════════════════════════════════════════════
   Producer-Consumer via PIPE
   ══════════════════════════════════════════════ */
void ipc_producer_consumer_demo(void) {
    int pipefd[2];
    if (pipe(pipefd) < 0) { perror("pipe"); return; }

    printf("\n  [IPC Demo] Producer-Consumer via Pipe\n");
    printf("  ─────────────────────────────────────\n");

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        /* Producer */
        close(pipefd[0]);
        for (int i = 1; i <= 5; i++) {
            char buf[32];
            int len = snprintf(buf, sizeof(buf), "item_%d", i);
            write(pipefd[1], buf, (size_t)len + 1);
            printf("  [Producer] Produced: %s\n", buf);
            fflush(stdout);
            usleep(200000);
        }
        close(pipefd[1]);
        exit(0);
    } else {
        /* Consumer */
        close(pipefd[1]);
        char buf[32];
        ssize_t n;
        while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
            printf("  [Consumer] Consumed: %s\n", buf);
            fflush(stdout);
        }
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        printf("  [IPC Demo] Producer-Consumer complete.\n\n");
        LOG_INFO("IPC: producer-consumer demo done");
    }
}

/* ══════════════════════════════════════════════
   Message queue style IPC using POSIX shm
   ══════════════════════════════════════════════ */
#define MQ_SHM  "/lunuxos_mq"
#define MQ_SIZE 16

typedef struct {
    char  data[64];
    int   valid;
} MQSlot;

typedef struct {
    MQSlot    slots[MQ_SIZE];
    int       head, tail, count;
    pthread_mutex_t lock;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} MsgQueue;

void ipc_message_queue_demo(void) {
    printf("\n  [IPC Demo] Message Queue (Shared Memory)\n");
    printf("  ─────────────────────────────────────────\n");

    int fd = shm_open(MQ_SHM, O_CREAT | O_RDWR, 0666);
    if (fd < 0) { perror("shm_open"); return; }
    ftruncate(fd, sizeof(MsgQueue));
    MsgQueue *mq = (MsgQueue *)mmap(NULL, sizeof(MsgQueue),
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED, fd, 0);
    close(fd);
    if (mq == MAP_FAILED) { perror("mmap"); return; }

    memset(mq, 0, sizeof(MsgQueue));
    pthread_mutexattr_t ma; pthread_mutexattr_init(&ma);
    pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mq->lock, &ma);
    pthread_mutexattr_destroy(&ma);

    pthread_condattr_t ca; pthread_condattr_init(&ca);
    pthread_condattr_setpshared(&ca, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&mq->not_empty, &ca);
    pthread_cond_init(&mq->not_full,  &ca);
    pthread_condattr_destroy(&ca);

    pid_t pid = fork();
    if (pid == 0) {
        /* Child: sender */
        const char *msgs[] = {"Hello","from","IPC","demo","END"};
        for (int i = 0; i < 5; i++) {
            pthread_mutex_lock(&mq->lock);
            while (mq->count == MQ_SIZE)
                pthread_cond_wait(&mq->not_full, &mq->lock);
            strncpy(mq->slots[mq->tail].data, msgs[i], 63);
            mq->slots[mq->tail].valid = 1;
            mq->tail  = (mq->tail + 1) % MQ_SIZE;
            mq->count++;
            pthread_cond_signal(&mq->not_empty);
            pthread_mutex_unlock(&mq->lock);
            printf("  [Sender]   Sent: %s\n", msgs[i]);
            fflush(stdout);
            usleep(150000);
        }
        munmap(mq, sizeof(MsgQueue));
        exit(0);
    } else {
        /* Parent: receiver */
        for (int i = 0; i < 5; i++) {
            pthread_mutex_lock(&mq->lock);
            while (mq->count == 0)
                pthread_cond_wait(&mq->not_empty, &mq->lock);
            char buf[64];
            strncpy(buf, mq->slots[mq->head].data, 63);
            mq->slots[mq->head].valid = 0;
            mq->head  = (mq->head + 1) % MQ_SIZE;
            mq->count--;
            pthread_cond_signal(&mq->not_full);
            pthread_mutex_unlock(&mq->lock);
            printf("  [Receiver] Received: %s\n", buf);
            fflush(stdout);
        }
        waitpid(pid, NULL, 0);
        munmap(mq, sizeof(MsgQueue));
        shm_unlink(MQ_SHM);
        printf("  [IPC Demo] Message queue demo complete.\n\n");
        LOG_INFO("IPC: message queue demo done");
    }
}

void ipc_run_demo(void) {
    ipc_producer_consumer_demo();
    ipc_message_queue_demo();
}
