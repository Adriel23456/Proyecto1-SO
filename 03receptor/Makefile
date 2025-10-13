# ================= RECEPTOR =================
# Compila receptor en Linux con semáforos POSIX y SHM System V

# ---------- Directorios ----------
INCDIR  := include
SRCDIR  := src
BINDIR  := bin
OBJDIR  := obj
OUTDIR  := out
TARGET  := $(BINDIR)/receptor

# ---------- Compilador y flags ----------
CC       := gcc
CSTD     := c11
WARN     := -Wall -Wextra -Wpedantic
OPT      := -O2
DEFS     := -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

CFLAGS   := $(WARN) $(OPT) -std=$(CSTD) $(DEFS)
CPPFLAGS := -I$(INCDIR)
LDFLAGS  := -pthread -lrt

# ---------- Colores ----------
RED      := \033[0;31m
GREEN    := \033[0;32m
YELLOW   := \033[0;33m
BLUE     := \033[0;34m
CYAN     := \033[0;36m
RESET    := \033[0m
BOLD     := \033[1m

# ---------- Archivos fuente y objetos ----------
SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

# ---------- Reglas principales ----------
.PHONY: all clean dirs run-auto run-manual run-auto-key run-manual-key status \
        watch-sleep strace kill-all help debug asan ubsan test

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BINDIR) $(OBJDIR) $(OUTDIR)

# Compilación con dependencias automáticas
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "$(CYAN)→ Compilando $<...$(RESET)"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	@echo "$(BOLD)$(BLUE)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(BLUE)║           Enlazando receptor...            ║$(RESET)"
	@echo "$(BOLD)$(BLUE)╚════════════════════════════════════════════╝$(RESET)"
	@$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "$(GREEN)✓ Ejecutable creado: $(TARGET)$(RESET)"
	@echo ""

clean:
	@echo "$(YELLOW)→ Limpiando archivos compilados...$(RESET)"
	@rm -rf $(OBJDIR) $(BINDIR)
	@echo "$(GREEN)✓ Limpieza completada$(RESET)"

clean-all: clean
	@echo "$(YELLOW)→ Limpiando archivos de salida...$(RESET)"
	@rm -rf $(OUTDIR)
	@echo "$(GREEN)✓ Limpieza completa$(RESET)"

# ---- Ejecución ----
run-auto: all
	@$(TARGET) auto

run-manual: all
	@$(TARGET) manual

run-auto-key: all
	@test -n "$(K)" || (echo "$(RED)Uso: make run-auto-key K=AA [D=100]$(RESET)"; exit 2)
	@$(TARGET) auto $(K) $(D)

run-manual-key: all
	@test -n "$(K)" || (echo "$(RED)Uso: make run-manual-key K=AA$(RESET)"; exit 2)
	@$(TARGET) manual $(K)

# ---- Estado y diagnóstico ----
status:
	@echo "$(BOLD)$(CYAN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(CYAN)║        Receptores Activos en el Sistema   ║$(RESET)"
	@echo "$(BOLD)$(CYAN)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@pgrep receptor | xargs -r -I{} ps -o pid,stat,pcpu,wchan,cmd -p {} || echo "  No hay receptores activos"
	@echo ""
	@echo "$(YELLOW)→ Semáforos POSIX:$(RESET)"
	@ls -l /dev/shm/sem.* 2>/dev/null || echo "  No encontrados"

watch-sleep:
	@echo "$(CYAN)Monitoreo en vivo de receptores (Ctrl+C para salir)$(RESET)"
	@echo "$(YELLOW)Columna wchan muestra el estado: futex=bloqueado, do_nanosleep=durmiendo$(RESET)"
	@echo ""
	@watch -n 0.5 'pgrep receptor | xargs -r -I{} ps -o pid,stat,pcpu,wchan,cmd -p {}'

strace:
	@test -n "$(PID)" || (echo "$(RED)Uso: make strace PID=<pid>$(RESET)"; exit 2)
	@echo "$(MAGENTA)→ Ejecutando strace en PID $(PID)...$(RESET)"
	@echo "$(YELLOW)Verá llamadas a futex (semáforos POSIX) - Ctrl+C para salir$(RESET)"
	@sudo strace -tt -p $(PID) -e trace=futex,ppoll,select,clock_nanosleep,nanosleep

kill-all:
	@echo "$(BOLD)$(RED)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(RED)║      Terminando Receptores (SIGUSR1)      ║$(RESET)"
	@echo "$(BOLD)$(RED)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@pgrep receptor | xargs -r -I{} kill -USR1 {} || echo "$(YELLOW)No había receptores activos$(RESET)"
	@echo "$(GREEN)✓ Señal SIGUSR1 enviada a todos los receptores$(RESET)"

# ---- Test ----
test: all
	@echo "$(BOLD)$(MAGENTA)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(MAGENTA)║              Test Rápido del Receptor      ║$(RESET)"
	@echo "$(BOLD)$(MAGENTA)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@echo "$(CYAN)→ Verificando memoria compartida...$(RESET)"
	@ipcs -m | grep 0x1234 > /dev/null && echo "$(GREEN)✓ Memoria compartida encontrada$(RESET)" || echo "$(RED)✗ Memoria compartida no encontrada. Ejecute el inicializador primero.$(RESET)"
	@echo ""
	@echo "$(CYAN)→ Verificando semáforos POSIX...$(RESET)"
	@ls /dev/shm/sem.* 2>/dev/null > /dev/null && echo "$(GREEN)✓ Semáforos POSIX encontrados$(RESET)" || echo "$(RED)✗ Semáforos no encontrados. Ejecute el inicializador primero.$(RESET)"
	@echo ""
	@echo "$(CYAN)→ Lanzando receptor de prueba por 5 segundos...$(RESET)"
	@timeout 5 $(TARGET) auto 2>/dev/null || true
	@echo ""
	@echo "$(GREEN)✓ Test completado$(RESET)"

# ---- Ayuda ----
help:
	@echo "$(BOLD)$(CYAN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(CYAN)║            Comandos Disponibles            ║$(RESET)"
	@echo "$(BOLD)$(CYAN)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@echo "$(GREEN)make$(RESET)              - Compilar el programa"
	@echo "$(GREEN)make run-auto$(RESET)     - Ejecutar en modo automático"
	@echo "$(GREEN)make run-manual$(RESET)   - Ejecutar en modo manual"
	@echo "$(GREEN)make run-auto-key K=AA D=100$(RESET) - Auto con clave y delay"
	@echo "$(GREEN)make run-manual-key K=AA$(RESET)     - Manual con clave"
	@echo "$(GREEN)make status$(RESET)       - Ver receptores activos y estado"
	@echo "$(GREEN)make watch-sleep$(RESET)  - Monitoreo en vivo (demuestra no busy-wait)"
	@echo "$(GREEN)make strace PID=<pid>$(RESET) - Strace de un receptor"
	@echo "$(GREEN)make kill-all$(RESET)     - Terminar todos los receptores"
	@echo "$(GREEN)make test$(RESET)         - Test rápido del sistema"
	@echo "$(GREEN)make clean$(RESET)        - Limpiar archivos compilados"
	@echo "$(GREEN)make clean-all$(RESET)    - Limpiar todo incluyendo salidas"
	@echo "$(GREEN)make help$(RESET)         - Mostrar esta ayuda"
	@echo ""

# ---- Perfiles de debugging ----
debug: CFLAGS += -O0 -g
debug: clean all

asan: CFLAGS += -O1 -g -fsanitize=address
asan: LDFLAGS += -fsanitize=address
asan: clean all

ubsan: CFLAGS += -O1 -g -fsanitize=undefined
ubsan: LDFLAGS += -fsanitize=undefined
ubsan: clean all