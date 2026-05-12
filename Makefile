# ═══════════════════════════════════════════════════════════
#   LunuxOS Makefile
#   Build:  make
#   Run:    make run   OR   ./lunuxos
#   Clean:  make clean
# ═══════════════════════════════════════════════════════════

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -I./include
LDFLAGS := -lpthread -lrt

# ── Directories ─────────────────────────────────────────────
BUILDDIR := build
LOGDIR   := logs

# ── Main OS binary ──────────────────────────────────────────
MAIN_BIN    := lunuxos
MAIN_SRC    := main.c \
               kernel/kernel.c \
               scheduler/scheduler.c \
               memory/memory.c \
               ipc/ipc.c \
               synchronization/sync.c \
               deadlock/deadlock.c

# ── Task binaries ────────────────────────────────────────────
TASKS := \
    task_calculator \
    task_notepad \
    task_clock \
    task_calendar \
    task_file_creator \
    task_file_copier \
    task_file_mover \
    task_file_deleter \
    task_file_info \
    task_shell \
    task_text_editor \
    task_beep \
    task_stopwatch \
    task_timer \
    task_typing \
    task_guessing \
    task_tictactoe \
    task_minesweeper \
    task_ram_monitor \
    task_cpu_monitor

TASK_BINS := $(addprefix $(BUILDDIR)/, $(TASKS))

# ── Default target ───────────────────────────────────────────
.PHONY: all clean run dirs

all: dirs $(MAIN_BIN) $(TASK_BINS)
	@echo ""
	@echo "  ╔══════════════════════════════════════╗"
	@echo "  ║   LunuxOS build complete!            ║"
	@echo "  ║   Run with:  ./lunuxos               ║"
	@echo "  ╚══════════════════════════════════════╝"
	@echo ""

# ── Create directories ───────────────────────────────────────
dirs:
	@mkdir -p $(BUILDDIR) $(LOGDIR)

# ── Main OS binary ───────────────────────────────────────────
$(MAIN_BIN): $(MAIN_SRC) include/lunuxos.h include/logger.h include/kernel.h \
             include/scheduler.h include/memory.h include/ipc.h \
             include/sync.h include/deadlock.h
	@echo "  [CC] Building main LunuxOS binary..."
	$(CC) $(CFLAGS) -o $@ $(MAIN_SRC) $(LDFLAGS)
	@echo "  [OK] lunuxos"

# ── Task binaries (generic rule) ─────────────────────────────
$(BUILDDIR)/task_%: tasks/task_%.c include/lunuxos.h include/logger.h include/task_common.h
	@echo "  [CC] Building task: $*"
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "  [OK] build/task_$*"

# ── Run ──────────────────────────────────────────────────────
run: all
	@echo "  Starting LunuxOS..."
	./lunuxos

# ── Clean ────────────────────────────────────────────────────
clean:
	@echo "  Cleaning build artifacts..."
	rm -f $(MAIN_BIN)
	rm -f $(BUILDDIR)/task_*
	rm -f /tmp/lunuxos_*.log
	rm -f /tmp/lunuxos_notepad.txt
	@shm_unlink_util() { true; }; true
	@echo "  Clean complete."
