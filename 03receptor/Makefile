# ================= RECEPTOR =================
# Compila receptor en Linux con semáforos POSIX y SHM System V

# ---------- Layout (defínelos primero) ----------
INCDIR  := include
SRCDIR  := src
BINDIR  := bin
OBJDIR  := obj
TARGET  := $(BINDIR)/receptor

# ---------- Toolchain ----------
CC      := gcc
CSTD    := c11
WARN    := -Wall -Wextra -Wpedantic
OPT     := -O2
DEFS    := -D_POSIX_C_SOURCE=200809L

CFLAGS   := $(WARN) $(OPT) -std=$(CSTD) $(DEFS)
CPPFLAGS := -I$(INCDIR)
DEPFLAGS := -MMD -MP
LDFLAGS  := -pthread -lrt

# ---------- Fuentes ----------
SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)

.PHONY: all clean dirs run-auto run-manual run-auto-key run-manual-key status \
        watch-sleep strace kill-all help debug asan ubsan

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BINDIR) $(OBJDIR)

# Compilación con dependencias automáticas
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(wildcard $(INCDIR)/*.h)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

-include $(DEPS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

# ---- Helpers de ejecución/diagnóstico ----
run-auto: all
	$(TARGET) auto

run-manual: all
	$(TARGET) manual

run-auto-key: all
	@test -n "$(K)" || (echo "Uso: make run-auto-key K=AA [D=100]"; exit 2)
	$(TARGET) auto $(K) $(D)

run-manual-key: all
	@test -n "$(K)" || (echo "Uso: make run-manual-key K=AA"; exit 2)
	$(TARGET) manual $(K)

status:
	@echo "Receptores activos:" ; \
	pgrep receptor | xargs -r -I{} ps -o pid,stat,pcpu,wchan,cmd -p {}
	@echo ; echo "Semáforos POSIX:" ; \
	ls -l /dev/shm/sem.* 2>/dev/null || true

watch-sleep:
	@echo "Ctrl+C para salir"
	watch -n 0.5 'pgrep receptor | xargs -r -I{} ps -o pid,stat,pcpu,wchan,cmd -p {}'

strace:
	@test -n "$(PID)" || (echo "Uso: make strace PID=<pid>"; exit 2)
	sudo strace -tt -p $(PID) -e trace=futex,ppoll,select,clock_nanosleep,nanosleep

kill-all:
	@pgrep receptor | xargs -r -n1 -I{} kill -USR1 {}
	@echo "SIGUSR1 enviado a todos los receptores (finalización elegante)."

help:
	@echo "Targets: make, run-auto, run-manual, run-auto-key, run-manual-key,"
	@echo "         status, watch-sleep, strace, kill-all, clean"
	@echo "Vars:    K=AA  D=100"

# ---- Perfiles extra ----
debug: CFLAGS += -O0 -g
debug: clean all

asan: CFLAGS += -O1 -g -fsanitize=address
asan: LDFLAGS += -fsanitize=address
asan: clean all

ubsan: CFLAGS += -O1 -g -fsanitize=undefined
ubsan: LDFLAGS += -fsanitize=undefined
ubsan: clean all
