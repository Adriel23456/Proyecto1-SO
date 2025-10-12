# Makefile para el Emisor
# Sistema de Comunicación entre Procesos con Memoria Compartida

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -std=gnu11
LDFLAGS = -pthread -lrt

# Directorios
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Nombre del ejecutable
TARGET = emisor

# Archivos fuente y objetos
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
HEADERS = $(wildcard $(INCDIR)/*.h)

# Colores para output
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[0;33m
BLUE = \033[0;34m
MAGENTA = \033[0;35m
CYAN = \033[0;36m
WHITE = \033[0;37m
RESET = \033[0m
BOLD = \033[1m

# Regla principal
all: directories $(TARGET)

# Crear directorios necesarios
directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Compilar el ejecutable
$(TARGET): $(OBJECTS)
	@echo "$(BOLD)$(BLUE)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(BLUE)║           Enlazando $(TARGET)...            ║$(RESET)"
	@echo "$(BOLD)$(BLUE)╚════════════════════════════════════════════╝$(RESET)"
	@$(CC) $(OBJECTS) -o $(BINDIR)/$(TARGET) $(LDFLAGS)
	@echo "$(GREEN)✓ Ejecutable creado: $(BINDIR)/$(TARGET)$(RESET)"
	@echo ""

# Compilar archivos objeto
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@echo "$(CYAN)→ Compilando $<...$(RESET)"
	@$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Ejecutar en modo automático
run-auto: $(TARGET)
	@echo "$(BOLD)$(GREEN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(GREEN)║    Ejecutando Emisor (Modo Automático)    ║$(RESET)"
	@echo "$(BOLD)$(GREEN)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@$(BINDIR)/$(TARGET) auto

# Ejecutar en modo automático con clave personalizada
run-auto-custom: $(TARGET)
	@echo "$(YELLOW)Ingrese la clave de encriptación (hex 2 caracteres):$(RESET)"
	@read -p "Clave: " key; \
	$(BINDIR)/$(TARGET) auto $$key

# Ejecutar en modo manual
run-manual: $(TARGET)
	@echo "$(BOLD)$(GREEN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(GREEN)║      Ejecutando Emisor (Modo Manual)      ║$(RESET)"
	@echo "$(BOLD)$(GREEN)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@$(BINDIR)/$(TARGET) manual

# Ejecutar múltiples emisores en paralelo
run-multiple: $(TARGET)
	@echo "$(YELLOW)¿Cuántos emisores desea lanzar?$(RESET)"
	@read -p "Número: " num; \
	for i in $$(seq 1 $$num); do \
		echo "$(CYAN)→ Lanzando emisor $$i...$(RESET)"; \
		$(BINDIR)/$(TARGET) auto & \
	done; \
	echo "$(GREEN)✓ $$num emisores lanzados$(RESET)"; \
	wait

# Ejecutar con delay personalizado
run-delay: $(TARGET)
	@echo "$(YELLOW)Ingrese el delay en milisegundos (10-5000):$(RESET)"
	@read -p "Delay: " delay; \
	$(BINDIR)/$(TARGET) auto AA $$delay

# Limpiar archivos compilados
clean:
	@echo "$(YELLOW)→ Limpiando archivos compilados...$(RESET)"
	@rm -rf $(OBJDIR) $(BINDIR)
	@echo "$(GREEN)✓ Limpieza completada$(RESET)"

# Debugging con valgrind
debug: $(TARGET)
	@echo "$(MAGENTA)→ Ejecutando con Valgrind...$(RESET)"
	@valgrind --leak-check=full --show-leak-kinds=all \
		--track-origins=yes $(BINDIR)/$(TARGET) auto

# Debugging con gdb
gdb: $(TARGET)
	@echo "$(MAGENTA)→ Ejecutando con GDB...$(RESET)"
	@gdb $(BINDIR)/$(TARGET)

# Ver procesos emisores activos
status:
	@echo "$(BOLD)$(CYAN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(CYAN)║        Emisores Activos en el Sistema     ║$(RESET)"
	@echo "$(BOLD)$(CYAN)╚════════════════════════════════════════════╝$(RESET)"
	@ps aux | grep -E "$(TARGET)|PID" | grep -v grep || echo "No hay emisores activos"

# Matar todos los emisores
kill-all:
	@echo "$(RED)→ Terminando todos los emisores...$(RESET)"
	@pkill -f $(TARGET) 2>/dev/null || echo "No había emisores activos"
	@echo "$(GREEN)✓ Emisores terminados$(RESET)"

# Ayuda
help:
	@echo "$(BOLD)$(CYAN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(CYAN)║            Comandos Disponibles            ║$(RESET)"
	@echo "$(BOLD)$(CYAN)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@echo "$(GREEN)make$(RESET)              - Compilar el programa"
	@echo "$(GREEN)make run-auto$(RESET)     - Ejecutar en modo automático"
	@echo "$(GREEN)make run-manual$(RESET)   - Ejecutar en modo manual"
	@echo "$(GREEN)make run-multiple$(RESET) - Lanzar múltiples emisores"
	@echo "$(GREEN)make run-delay$(RESET)    - Ejecutar con delay personalizado"
	@echo "$(GREEN)make clean$(RESET)        - Limpiar archivos compilados"
	@echo "$(GREEN)make debug$(RESET)        - Ejecutar con Valgrind"
	@echo "$(GREEN)make gdb$(RESET)          - Ejecutar con GDB"
	@echo "$(GREEN)make status$(RESET)       - Ver emisores activos"
	@echo "$(GREEN)make kill-all$(RESET)     - Terminar todos los emisores"
	@echo "$(GREEN)make help$(RESET)         - Mostrar esta ayuda"
	@echo ""

# Test rápido del sistema
test: $(TARGET)
	@echo "$(BOLD)$(MAGENTA)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(MAGENTA)║              Test Rápido del Emisor        ║$(RESET)"
	@echo "$(BOLD)$(MAGENTA)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@echo "$(CYAN)→ Verificando memoria compartida...$(RESET)"
	@ipcs -m | grep 0x1234 > /dev/null && echo "$(GREEN)✓ Memoria compartida encontrada$(RESET)" || echo "$(RED)✗ Memoria compartida no encontrada. Ejecute el inicializador primero.$(RESET)"
	@echo ""
	@echo "$(CYAN)→ Verificando semáforos POSIX...$(RESET)"
	@ls /dev/shm/sem.sem_* 2>/dev/null > /dev/null && echo "$(GREEN)✓ Semáforos POSIX encontrados$(RESET)" || echo "$(RED)✗ Semáforos no encontrados. Ejecute el inicializador primero.$(RESET)"
	@echo ""
	@echo "$(CYAN)→ Lanzando emisor de prueba por 5 segundos...$(RESET)"
	@timeout 5 $(BINDIR)/$(TARGET) auto 2>/dev/null || true
	@echo ""
	@echo "$(GREEN)✓ Test completado$(RESET)"

# Phony targets
.PHONY: all directories clean run-auto run-manual run-multiple run-delay debug gdb status kill-all help test run-auto-custom

# Regla por defecto
.DEFAULT_GOAL := all