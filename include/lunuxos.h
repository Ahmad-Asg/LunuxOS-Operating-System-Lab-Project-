#ifndef LUNUXOS_H
#define LUNUXOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <libgen.h>

/* ══════════════════════════════════════════════
   OS Identity
   ══════════════════════════════════════════════ */
#define OS_NAME      "LunuxOS"
#define OS_VERSION   "1.0.0"

/* ══════════════════════════════════════════════
   Shared Memory & Logging
   ══════════════════════════════════════════════ */
#define SHM_NAME     "/lunuxos_state"
#define LOG_FILE     "/tmp/lunuxos_system.log"

/* ══════════════════════════════════════════════
   Limits
   ══════════════════════════════════════════════ */
#define MAX_PROCESSES 64
#define MAX_NAME_LEN  64
#define MAX_PATH_LEN  256
#define TIME_QUANTUM  2   /* Round Robin quantum (seconds simulated) */

/* ══════════════════════════════════════════════
   Task RAM Requirements (MB)
   ══════════════════════════════════════════════ */
#define RAM_CALCULATOR   32
#define RAM_NOTEPAD      64
#define RAM_CLOCK        16
#define RAM_CALENDAR     16
#define RAM_FILE_OPS     32
#define RAM_SHELL        48
#define RAM_EDITOR       64
#define RAM_BEEP         16
#define RAM_STOPWATCH    16
#define RAM_TIMER        16
#define RAM_TYPING       32
#define RAM_GUESSING     16
#define RAM_TICTACTOE    16
#define RAM_MINESWEEPER  32
#define RAM_MONITOR      16

/* ══════════════════════════════════════════════
   Process States
   ══════════════════════════════════════════════ */
typedef enum {
    PROC_NEW        = 0,
    PROC_READY      = 1,
    PROC_RUNNING    = 2,
    PROC_WAITING    = 3,
    PROC_BLOCKED    = 4,
    PROC_TERMINATED = 5
} ProcState;

/* ══════════════════════════════════════════════
   Queue Levels (Multi-Level Queue)
   ══════════════════════════════════════════════ */
typedef enum {
    QUEUE_RR   = 1,  /* Round Robin    – interactive tasks  */
    QUEUE_FCFS = 2,  /* FCFS           – batch tasks        */
    QUEUE_PRIO = 3   /* Priority Sched – background tasks   */
} QueueLevel;

/* ══════════════════════════════════════════════
   Process Control Block
   ══════════════════════════════════════════════ */
typedef struct {
    pid_t      pid;
    char       name[MAX_NAME_LEN];
    ProcState  state;
    int        priority;        /* 1-10 (10 = highest) */
    QueueLevel queue;
    int        ram_mb;
    int        storage_mb;
    int        valid;           /* 1 = slot occupied   */
    time_t     created_at;
    int        burst_time;
    int        remaining_time;
    int        wait_time;
    int        age;             /* for starvation prevention */
} PCB;

/* ══════════════════════════════════════════════
   System State  –  lives in POSIX shared memory
   ══════════════════════════════════════════════ */
typedef struct {
    /* ── Resources ── */
    int total_ram_mb;
    int used_ram_mb;
    int total_storage_mb;
    int used_storage_mb;
    int total_cores;
    int used_cores;

    /* ── Process table ── */
    PCB proc_table[MAX_PROCESSES];
    int proc_count;

    /* ── System flags ── */
    volatile int kernel_running;
    volatile int kernel_mode;
    volatile int shutdown_requested;

    /* ── Sync (PTHREAD_PROCESS_SHARED) ── */
    pthread_mutex_t table_lock;
    pthread_mutex_t resource_lock;

    /* ── Statistics ── */
    int total_created;
    int total_terminated;
    int deadlock_count;

    /* ── Simple queue snapshots (indices into proc_table) ── */
    int ready_q[MAX_PROCESSES];
    int ready_q_count;
    int wait_q[MAX_PROCESSES];
    int wait_q_count;
    int blocked_q[MAX_PROCESSES];
    int blocked_q_count;

} SystemState;

#endif /* LUNUXOS_H */
