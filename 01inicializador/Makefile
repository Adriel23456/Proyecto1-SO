# Makefile para el Inicializador
# Sistema de Comunicación entre Procesos con Memoria Compartida

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -std=gnu11
LDFLAGS = -lpthread

# Directorios
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin
ASSETSDIR = assets

# Nombre del ejecutable
TARGET = inicializador

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
all: directories $(TARGET) assets

# Crear directorios necesarios
directories:
	@mkdir -p $(OBJDIR) $(BINDIR) $(ASSETSDIR)

# Compilar el ejecutable
$(TARGET): $(OBJECTS)
	@echo "$(BOLD)$(BLUE)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(BLUE)║         Enlazando $(TARGET)...         ║$(RESET)"
	@echo "$(BOLD)$(BLUE)╚════════════════════════════════════════════╝$(RESET)"
	@$(CC) $(OBJECTS) -o $(BINDIR)/$(TARGET) $(LDFLAGS)
	@echo "$(GREEN)✓ Ejecutable creado: $(BINDIR)/$(TARGET)$(RESET)"
	@echo ""

# Compilar archivos objeto
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@echo "$(CYAN)→ Compilando $<...$(RESET)"
	@$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Crear archivo de prueba si no existe
assets:
	@if [ ! -f $(ASSETSDIR)/data.txt ]; then \
		echo "$(YELLOW)→ Creando archivo de prueba...$(RESET)"; \
		echo "Hello World!" > $(ASSETSDIR)/data.txt; \
		echo "This is a test file for the shared memory system." >> $(ASSETSDIR)/data.txt; \
		echo "Each character will be encrypted and transmitted." >> $(ASSETSDIR)/data.txt; \
		echo "The system guarantees sequential order!" >> $(ASSETSDIR)/data.txt; \
		echo "$(GREEN)✓ Archivo de prueba creado: $(ASSETSDIR)/data.txt$(RESET)"; \
	fi

# Ejecutar el programa con parámetros de ejemplo
run: all
	@echo "$(BOLD)$(GREEN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(GREEN)║         Ejecutando Inicializador...        ║$(RESET)"
	@echo "$(BOLD)$(GREEN)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@cd $(BINDIR) && ./$(TARGET) ../$(ASSETSDIR)/data.txt 10 AA

# Ejecutar con parámetros personalizados
run-custom: all
	@echo "$(YELLOW)Ingrese los parámetros:$(RESET)"
	@echo "Uso: <archivo> <tamaño_buffer> <clave_hex>"
	@read -p "Parámetros: " params; \
	cd $(BINDIR) && ./$(TARGET) $$params

# Limpiar archivos compilados
clean:
	@echo "$(YELLOW)→ Limpiando archivos compilados...$(RESET)"
	@rm -rf $(OBJDIR) $(BINDIR)
	@echo "$(GREEN)✓ Limpieza completada$(RESET)"

# Limpiar todo incluyendo archivos generados
clean-all: clean
	@echo "$(YELLOW)→ Limpiando archivos generados...$(RESET)"
	@rm -f $(ASSETSDIR)/*.bin
	@rm -f $(ASSETSDIR)/data.txt
	@echo "$(GREEN)✓ Limpieza completa$(RESET)"

# Limpiar memoria compartida y semáforos (usar con cuidado)
clean-ipc:
	@echo "$(RED)$(BOLD)⚠ ADVERTENCIA: Esto eliminará TODA la memoria compartida y semáforos$(RESET)"
	@read -p "¿Está seguro? (y/N): " confirm; \
	if [ "$$confirm" = "y" ] || [ "$$confirm" = "Y" ]; then \
		echo "$(YELLOW)→ Limpiando IPC...$(RESET)"; \
		ipcrm -M 0x1234 2>/dev/null || true; \
		ipcrm -S 0x5000 2>/dev/null || true; \
		echo "$(GREEN)✓ IPC limpiado$(RESET)"; \
	else \
		echo "$(CYAN)Operación cancelada$(RESET)"; \
	fi

# Ver estado de la memoria compartida y semáforos
status:
	@echo "$(BOLD)$(CYAN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(CYAN)║        Estado del Sistema IPC              ║$(RESET)"
	@echo "$(BOLD)$(CYAN)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@echo "$(YELLOW)→ Memoria Compartida:$(RESET)"
	@ipcs -m | grep -E "0x00001234|key" || echo "  No hay memoria compartida activa"
	@echo ""
	@echo "$(YELLOW)→ Semáforos:$(RESET)"
	@ipcs -s | grep -E "0x00005000|key" || echo "  No hay semáforos activos"
	@echo ""

# Debugging con valgrind
debug: all
	@echo "$(MAGENTA)→ Ejecutando con Valgrind...$(RESET)"
	@cd $(BINDIR) && valgrind --leak-check=full --show-leak-kinds=all \
		--track-origins=yes ./$(TARGET) ../$(ASSETSDIR)/data.txt 10 AA

# Debugging con gdb
gdb: all
	@echo "$(MAGENTA)→ Ejecutando con GDB...$(RESET)"
	@cd $(BINDIR) && gdb ./$(TARGET)

# Ayuda
help:
	@echo "$(BOLD)$(CYAN)╔════════════════════════════════════════════╗$(RESET)"
	@echo "$(BOLD)$(CYAN)║            Comandos Disponibles            ║$(RESET)"
	@echo "$(BOLD)$(CYAN)╚════════════════════════════════════════════╝$(RESET)"
	@echo ""
	@echo "$(GREEN)make$(RESET)           - Compilar el programa"
	@echo "$(GREEN)make run$(RESET)       - Ejecutar con parámetros de ejemplo"
	@echo "$(GREEN)make run-custom$(RESET)- Ejecutar con parámetros personalizados"
	@echo "$(GREEN)make clean$(RESET)     - Limpiar archivos compilados"
	@echo "$(GREEN)make clean-all$(RESET) - Limpiar todo incluyendo archivos generados"
	@echo "$(GREEN)make clean-ipc$(RESET) - Limpiar memoria compartida y semáforos"
	@echo "$(GREEN)make status$(RESET)    - Ver estado del sistema IPC"
	@echo "$(GREEN)make debug$(RESET)     - Ejecutar con Valgrind"
	@echo "$(GREEN)make gdb$(RESET)       - Ejecutar con GDB"
	@echo "$(GREEN)make help$(RESET)      - Mostrar esta ayuda"
	@echo ""

# Phony targets
.PHONY: all directories run run-custom clean clean-all clean-ipc status debug gdb help assets

# Regla por defecto
.DEFAULT_GOAL := all