#!/usr/bin/env bash

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
RESET='\033[0m'
BOLD='\033[1m'

show_banner() {
    clear
    echo -e "${BOLD}${CYAN}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${CYAN}║                SETUP - FINALIZADOR                         ║${RESET}"
    echo -e "${BOLD}${CYAN}║         Sistema de Comunicación entre Procesos             ║${RESET}"
    echo -e "${BOLD}${CYAN}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
}

show_menu() {
    echo -e "${BOLD}${GREEN}Seleccione una opción:${RESET}"
    echo ""
    echo -e "${YELLOW}1)${RESET} Compilar proyecto"
    echo -e "${YELLOW}2)${RESET} Ejecutar finalizador"
    echo -e "${YELLOW}3)${RESET} Limpiar compilación"
    echo -e "${YELLOW}4)${RESET} Salir"
    echo ""
    echo -n "Opción: "
}

build_project() {
    echo -e "${BOLD}${BLUE}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${BLUE}║                  COMPILACIÓN DEL FINALIZADOR              ║${RESET}"
    echo -e "${BOLD}${BLUE}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    echo -e "${YELLOW}→ Limpiando...${RESET}"
    make clean
    
    echo -e "${YELLOW}→ Compilando...${RESET}"
    if make; then
        echo ""
        echo -e "${GREEN}✓ Finalizador compilado: bin/finalizador${RESET}"
    else
        echo ""
        echo -e "${RED}✗ Error de compilación${RESET}"
        return 1
    fi
}

run_finalizador() {
    if [ ! -f "bin/finalizador" ]; then
        echo -e "${RED}✗ Finalizador no compilado${RESET}"
        echo -e "${YELLOW}Compilando primero...${RESET}"
        build_project || return 1
    fi
    
    echo -e "${CYAN}→ Ejecutando finalizador...${RESET}"
    ./bin/finalizador
}

clean_project() {
    echo -e "${YELLOW}→ Limpiando compilación...${RESET}"
    make clean
    echo -e "${GREEN}✓ Limpieza completada${RESET}"
}

main() {
    while true; do
        show_banner
        show_menu
        read -r option
        
        case $option in
            1) build_project ;;
            2) run_finalizador ;;
            3) clean_project ;;
            4) echo ""; echo -e "${CYAN}¡Hasta luego!${RESET}"; exit 0 ;;
            *) echo -e "${RED}Opción inválida${RESET}" ;;
        esac
        
        echo ""
        echo -e "${YELLOW}Presione Enter para continuar...${RESET}"
        read -r
    done
}

chmod +x "$0"
main