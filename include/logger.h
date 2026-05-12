#ifndef LOGGER_H
#define LOGGER_H

#include "lunuxos.h"
#include <sys/file.h>

/* ══════════════════════════════════════════════
   Cross-process logging using flock()
   Works from main OS and from every task process
   ══════════════════════════════════════════════ */
static inline void log_event(const char *level, const char *fmt, ...) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;

    int fd = fileno(f);
    flock(fd, LOCK_EX);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", t);

    fprintf(f, "[%s] [%-5s] [PID:%-6d] ", tbuf, level, (int)getpid());

    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);

    fprintf(f, "\n");
    fflush(f);

    flock(fd, LOCK_UN);
    fclose(f);
}

#define LOG_INFO(fmt, ...)  log_event("INFO",  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_event("WARN",  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_event("ERROR", fmt, ##__VA_ARGS__)
#define LOG_SCHED(fmt, ...) log_event("SCHED", fmt, ##__VA_ARGS__)
#define LOG_MEM(fmt, ...)   log_event("MEM",   fmt, ##__VA_ARGS__)
#define LOG_DEAD(fmt, ...)  log_event("DEAD",  fmt, ##__VA_ARGS__)

#endif /* LOGGER_H */
