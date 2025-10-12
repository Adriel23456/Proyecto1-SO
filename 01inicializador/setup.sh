#!/bin/bash

# setup.sh - Script de configuración e instalación
# Sistema de Comunicación entre Procesos con Memoria Compartida

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[0;37m'
RESET='\033[0m'
BOLD='\033[1m'

# Banner del programa
show_banner() {
    clear
    echo -e "${BOLD}${CYAN}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${CYAN}║         SETUP - SISTEMA DE COMUNICACIÓN IPC                ║${RESET}"
    echo -e "${BOLD}${CYAN}║              Inicializador de Memoria Compartida           ║${RESET}"
    echo -e "${BOLD}${CYAN}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
}

# Menú principal
show_menu() {
    echo -e "${BOLD}${GREEN}Seleccione una opción:${RESET}"
    echo ""
    echo -e "${YELLOW}1)${RESET} Instalar dependencias"
    echo -e "${YELLOW}2)${RESET} Compilar proyecto"
    echo -e "${YELLOW}3)${RESET} Compilar y ejecutar con parámetros de ejemplo"
    echo -e "${YELLOW}4)${RESET} Limpiar archivos compilados"
    echo -e "${YELLOW}5)${RESET} Verificar estado del sistema IPC"
    echo -e "${YELLOW}6)${RESET} Limpiar memoria compartida y semáforos"
    echo -e "${YELLOW}7)${RESET} Instalación completa (dependencias + compilación)"
    echo -e "${YELLOW}8)${RESET} Verificar instalación"
    echo -e "${YELLOW}9)${RESET} Salir"
    echo ""
    echo -n "Opción: "
}

# Función para verificar si un comando existe
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Función para detectar el sistema operativo
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Detectar distribución específica
        if [ -f /etc/debian_version ]; then
            echo "debian"
        elif [ -f /etc/redhat-release ]; then
            echo "redhat"
        elif [ -f /etc/arch-release ]; then
            echo "arch"
        else
            echo "linux"
        fi
    else
        echo "unsupported"
    fi
}

# Instalar dependencias
install_dependencies() {
    echo -e "${BOLD}${BLUE}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${BLUE}║                 INSTALACIÓN DE DEPENDENCIAS                ║${RESET}"
    echo -e "${BOLD}${BLUE}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    OS=$(detect_os)
    
    echo -e "${CYAN}→ Sistema detectado: ${OS}${RESET}"
    echo ""
    
    case $OS in
        debian)
            echo -e "${YELLOW}→ Actualizando repositorios...${RESET}"
            sudo apt-get update
            
            echo -e "${YELLOW}→ Instalando herramientas de desarrollo...${RESET}"
            sudo apt-get install -y build-essential gcc make
            
            echo -e "${YELLOW}→ Instalando bibliotecas del sistema...${RESET}"
            sudo apt-get install -y libc6-dev
            
            echo -e "${YELLOW}→ Instalando herramientas de debugging (opcional)...${RESET}"
            sudo apt-get install -y gdb valgrind
            
            echo -e "${YELLOW}→ Instalando utilidades IPC...${RESET}"
            sudo apt-get install -y ipcs
            ;;
            
        redhat)
            echo -e "${YELLOW}→ Instalando grupo de desarrollo...${RESET}"
            sudo yum groupinstall -y "Development Tools"
            
            echo -e "${YELLOW}→ Instalando bibliotecas del sistema...${RESET}"
            sudo yum install -y glibc-devel
            
            echo -e "${YELLOW}→ Instalando herramientas de debugging (opcional)...${RESET}"
            sudo yum install -y gdb valgrind
            ;;
            
        arch)
            echo -e "${YELLOW}→ Actualizando sistema...${RESET}"
            sudo pacman -Syu
            
            echo -e "${YELLOW}→ Instalando herramientas de desarrollo...${RESET}"
            sudo pacman -S --needed base-devel gcc make
            
            echo -e "${YELLOW}→ Instalando herramientas de debugging (opcional)...${RESET}"
            sudo pacman -S --needed gdb valgrind
            ;;
            
        *)
            echo -e "${RED}⚠ Sistema no soportado automáticamente${RESET}"
            echo -e "${YELLOW}Por favor, instale manualmente:${RESET}"
            echo "  - GCC compiler"
            echo "  - Make"
            echo "  - Development headers for libc"
            echo "  - GDB y Valgrind (opcional)"
            return 1
            ;;
    esac
    
    echo ""
    echo -e "${GREEN}✓ Dependencias instaladas correctamente${RESET}"
}

# Compilar el proyecto
build_project() {
    echo -e "${BOLD}${BLUE}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${BLUE}║                  COMPILACIÓN DEL PROYECTO                  ║${RESET}"
    echo -e "${BOLD}${BLUE}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    # Verificar que gcc esté instalado
    if ! command_exists gcc; then
        echo -e "${RED}⚠ GCC no está instalado${RESET}"
        echo -e "${YELLOW}Ejecute la opción 1 para instalar dependencias${RESET}"
        return 1
    fi
    
    # Verificar que make esté instalado
    if ! command_exists make; then
        echo -e "${RED}⚠ Make no está instalado${RESET}"
        echo -e "${YELLOW}Ejecute la opción 1 para instalar dependencias${RESET}"
        return 1
    fi
    
    # Compilar usando make
    echo -e "${YELLOW}→ Limpiando compilaciones anteriores...${RESET}"
    make clean
    
    echo -e "${YELLOW}→ Compilando el proyecto...${RESET}"
    if make; then
        echo ""
        echo -e "${GREEN}✓ Proyecto compilado exitosamente${RESET}"
        echo -e "${CYAN}  Ejecutable: bin/inicializador${RESET}"
    else
        echo ""
        echo -e "${RED}✗ Error durante la compilación${RESET}"
        return 1
    fi
}

# Compilar y ejecutar
build_and_run() {
    if build_project; then
        echo ""
        echo -e "${YELLOW}→ Ejecutando con parámetros de ejemplo...${RESET}"
        echo ""
        make run
    fi
}

# Limpiar archivos
clean_project() {
    echo -e "${BOLD}${BLUE}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${BLUE}║                    LIMPIEZA DEL PROYECTO                   ║${RESET}"
    echo -e "${BOLD}${BLUE}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    echo -e "${YELLOW}→ Limpiando archivos compilados...${RESET}"
    make clean
    
    echo -e "${YELLOW}→ ¿Desea eliminar también los archivos generados (.bin)? (y/N)${RESET}"
    read -r response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        make clean-all
    fi
    
    echo ""
    echo -e "${GREEN}✓ Limpieza completada${RESET}"
}

# Ver estado del sistema IPC
check_ipc_status() {
    echo -e "${BOLD}${BLUE}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${BLUE}║                  ESTADO DEL SISTEMA IPC                    ║${RESET}"
    echo -e "${BOLD}${BLUE}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    make status
}

# Limpiar IPC
clean_ipc() {
    echo -e "${BOLD}${RED}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${RED}║                    LIMPIEZA DE IPC                         ║${RESET}"
    echo -e "${BOLD}${RED}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    make clean-ipc
}

# Instalación completa
full_install() {
    echo -e "${BOLD}${MAGENTA}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${MAGENTA}║                  INSTALACIÓN COMPLETA                      ║${RESET}"
    echo -e "${BOLD}${MAGENTA}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    install_dependencies
    echo ""
    
    if [ $? -eq 0 ]; then
        build_project
    else
        echo -e "${RED}✗ No se pudo completar la instalación${RESET}"
        return 1
    fi
    
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${GREEN}║            INSTALACIÓN COMPLETADA CON ÉXITO                ║${RESET}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${RESET}"
}

# Verificar instalación
verify_installation() {
    echo -e "${BOLD}${BLUE}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${BLUE}║                VERIFICACIÓN DE INSTALACIÓN                 ║${RESET}"
    echo -e "${BOLD}${BLUE}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    local all_ok=true
    
    # Verificar GCC
    echo -n -e "${YELLOW}→ Verificando GCC... ${RESET}"
    if command_exists gcc; then
        gcc_version=$(gcc --version | head -n1)
        echo -e "${GREEN}✓${RESET} ($gcc_version)"
    else
        echo -e "${RED}✗ No instalado${RESET}"
        all_ok=false
    fi
    
    # Verificar Make
    echo -n -e "${YELLOW}→ Verificando Make... ${RESET}"
    if command_exists make; then
        make_version=$(make --version | head -n1)
        echo -e "${GREEN}✓${RESET} ($make_version)"
    else
        echo -e "${RED}✗ No instalado${RESET}"
        all_ok=false
    fi
    
    # Verificar ipcs
    echo -n -e "${YELLOW}→ Verificando ipcs... ${RESET}"
    if command_exists ipcs; then
        echo -e "${GREEN}✓${RESET}"
    else
        echo -e "${YELLOW}⚠ No instalado (opcional)${RESET}"
    fi
    
    # Verificar GDB
    echo -n -e "${YELLOW}→ Verificando GDB... ${RESET}"
    if command_exists gdb; then
        gdb_version=$(gdb --version | head -n1)
        echo -e "${GREEN}✓${RESET} ($gdb_version)"
    else
        echo -e "${YELLOW}⚠ No instalado (opcional)${RESET}"
    fi
    
    # Verificar Valgrind
    echo -n -e "${YELLOW}→ Verificando Valgrind... ${RESET}"
    if command_exists valgrind; then
        valgrind_version=$(valgrind --version)
        echo -e "${GREEN}✓${RESET} ($valgrind_version)"
    else
        echo -e "${YELLOW}⚠ No instalado (opcional)${RESET}"
    fi
    
    # Verificar ejecutable
    echo -n -e "${YELLOW}→ Verificando ejecutable... ${RESET}"
    if [ -f "bin/inicializador" ]; then
        echo -e "${GREEN}✓${RESET} (bin/inicializador)"
    else
        echo -e "${YELLOW}⚠ No compilado${RESET}"
    fi
    
    echo ""
    
    if [ "$all_ok" = true ]; then
        echo -e "${GREEN}✓ Sistema listo para usar${RESET}"
    else
        echo -e "${YELLOW}⚠ Faltan componentes esenciales${RESET}"
        echo -e "${CYAN}  Ejecute la opción 7 para instalación completa${RESET}"
    fi
}

# Main loop
main() {
    while true; do
        show_banner
        show_menu
        
        read -r option
        
        case $option in
            1)
                install_dependencies
                ;;
            2)
                build_project
                ;;
            3)
                build_and_run
                ;;
            4)
                clean_project
                ;;
            5)
                check_ipc_status
                ;;
            6)
                clean_ipc
                ;;
            7)
                full_install
                ;;
            8)
                verify_installation
                ;;
            9)
                echo ""
                echo -e "${CYAN}¡Hasta luego!${RESET}"
                exit 0
                ;;
            *)
                echo -e "${RED}Opción inválida${RESET}"
                ;;
        esac
        
        echo ""
        echo -e "${YELLOW}Presione Enter para continuar...${RESET}"
        read -r
    done
}

# Hacer el script ejecutable
chmod +x "$0"

# Ejecutar main
main