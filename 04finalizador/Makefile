# ================= FINALIZADOR =================
# Compila el finalizador garantizando que se reconstruye cuando cambian headers
# y que usa las mismas cabeceras compartidas (structures.h) que el resto del proyecto.

# ---------- Directorios ----------
INCDIR   := include
SRCDIR   := src
BINDIR   := bin
OBJDIR   := obj
TARGET   := $(BINDIR)/finalizador

# Si tus headers compartidos viven en otros módulos, agrégalos aquí:
# (ajusta rutas según tu repo: por ejemplo ../receptor/include, ../inicializador/include)
EXTRA_INC_DIRS ?= ../receptor/include ../inicializador/include ../common/include

# ---------- Compilador y flags ----------
CC       := gcc
CSTD     := c11
WARN     := -Wall -Wextra -Wpedantic
OPT      := -O2
DEFS     := -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

CPPFLAGS := -I$(INCDIR) $(addprefix -I,$(EXTRA_INC_DIRS)) $(DEFS)
CFLAGS   := $(WARN) $(OPT) -std=$(CSTD) -MMD -MP
LDFLAGS  :=
LDLIBS   := -pthread -lrt

# Verbosidad (make V=1 para ver comandos)
V ?= 0
ifeq ($(V),0)
  Q := @
else
  Q :=
endif

# ---------- Colores ----------
RED      := \033[0;31m
GREEN    := \033[0;32m
YELLOW   := \033[0;33m
BLUE     := \033[0;34m
CYAN     := \033[0;36m
RESET    := \033[0m
BOLD     := \033[1m

# ---------- Fuentes / objetos / deps ----------
SOURCES  := $(wildcard $(SRCDIR)/*.c)
OBJECTS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
DEPFILES := $(OBJECTS:.o=.d)

# ---------- Reglas principales ----------
.PHONY: all clean clean-all dirs run rebuild help debug asan ubsan abi-check \
        clean-ipc status

all: dirs $(TARGET)

dirs:
	$(Q)mkdir -p $(BINDIR) $(OBJDIR)

# Compilación con dependencias automáticas (-MMD -MP)
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "$(CYAN)→ Compilando $<...$(RESET)"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	@echo "$(BOLD)$(BLUE)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(BLUE)║           Enlazando finalizador...         ║$(RESET)"
	@echo "$(BOLD)$(BLUE)╚════════════════════════════════════════════╝$(RESET)"
	$(Q)$(CC) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)
	@echo "$(GREEN)✓ Ejecutable creado: $(TARGET)$(RESET)"
	@echo ""

# ---------- Utilidades ----------
run: all
	$(Q)$(TARGET)

rebuild: clean all

clean:
	@echo "$(YELLOW)→ Limpiando objetos y binarios...$(RESET)"
	$(Q)rm -rf $(OBJDIR) $(BINDIR)
	@echo "$(GREEN)✓ Limpieza completada$(RESET)"

clean-all: clean clean-ipc

# Limpieza de IPC (tal cual pediste)
clean-ipc:
	@echo "$(BOLD)$(RED)╔══════════════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(RED)║                    LIMPIEZA DE IPC                   ║$(RESET)"
	@echo "$(BOLD)$(RED)╚══════════════════════════════════════════════════════╝$(RESET)"
	@ipcrm -M 0x1234 2>/dev/null || true
	@rm -f /dev/shm/sem.sem.ipc_* 2>/dev/null || true
	@echo "$(GREEN)✓ IPC limpiado (SHM y semáforos POSIX)$(RESET)"

# Muestra procesos finalizador activos (opcional)
status:
	@echo "$(BOLD)$(CYAN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(CYAN)║         Finalizadores activos (ps)         ║$(RESET)"
	@echo "$(BOLD)$(CYAN)╚════════════════════════════════════════════╝$(RESET)"
	@pgrep -a finalizador || echo "  No hay finalizadores activos"

# ---------- Perfiles de debugging ----------
debug: CFLAGS += -O0 -g
debug: rebuild

asan: CFLAGS += -O1 -g -fsanitize=address
asan: LDLIBS += -fsanitize=address
asan: rebuild

ubsan: CFLAGS += -O1 -g -fsanitize=undefined
ubsan: LDLIBS += -fsanitize=undefined
ubsan: rebuild

# ---------- ABI check (alineación de headers) ----------
# Compila y ejecuta un pequeño programa que imprime sizeof(SharedMemory)
# Útil para validar que el finalizador está incluyendo el MISMO structures.h
abi-check: dirs
	@echo "$(YELLOW)→ Verificando ABI (sizeof(SharedMemory))...$(RESET)"
	$(Q)printf '%s\n' \
		'#include <stdio.h>' \
		'#include "structures.h"' \
		'int main(void){printf("sizeof(SharedMemory) = %zu\\n", sizeof(SharedMemory));return 0;}' \
		> $(OBJDIR)/abi_check.c
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJDIR)/abi_check.c -o $(OBJDIR)/abi_check $(LDLIBS)
	$(Q)$(OBJDIR)/abi_check

help:
	@echo "$(BOLD)$(CYAN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(CYAN)║            Comandos Disponibles            ║$(RESET)"
	@echo "$(BOLD)$(CYAN)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@echo "$(GREEN)make$(RESET)            - Compilar finalizador"
	@echo "$(GREEN)make run$(RESET)        - Ejecutar finalizador"
	@echo "$(GREEN)make status$(RESET)     - Listar finalizadores activos"
	@echo "$(GREEN)make abi-check$(RESET)  - Verificar sizeof(SharedMemory)"
	@echo "$(GREEN)make clean$(RESET)      - Limpiar binarios y objetos"
	@echo "$(GREEN)make clean-ipc$(RESET)  - Limpiar SHM y semáforos POSIX"
	@echo "$(GREEN)make debug/asan/ubsan$(RESET) - Perfiles de depuración"
	@echo "$(GREEN)make rebuild$(RESET)    - Clean + build"
	@echo ""

# Incluir dependencias generadas (-MMD -MP)
-include $(DEPFILES)
