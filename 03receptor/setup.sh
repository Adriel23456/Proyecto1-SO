#!/usr/bin/env bash
# setup.sh - Script de configuración para el Receptor
# Sistema de Comunicación entre Procesos con Memoria Compartida

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
    echo -e "${BOLD}${CYAN}║                SETUP - RECEPTOR DEL SISTEMA IPC            ║${RESET}"
    echo -e "${BOLD}${CYAN}║         Sistema de Comunicación entre Procesos             ║${RESET}"
    echo -e "${BOLD}${CYAN}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
}

show_menu() {
    echo -e "${BOLD}${GREEN}Seleccione una opción:${RESET}"
    echo ""
    echo -e "${YELLOW}1)${RESET} Instalar dependencias"
    echo -e "${YELLOW}2)${RESET} Compilar proyecto"
    echo -e "${YELLOW}3)${RESET} Ejecutar en modo automático"
    echo -e "${YELLOW}4)${RESET} Ejecutar en modo manual"
    echo -e "${YELLOW}5)${RESET} Lanzar múltiples receptores"
    echo -e "${YELLOW}6)${RESET} Ver receptores activos"
    echo -e "${YELLOW}7)${RESET} Terminar todos los receptores (SIGUSR1)"
    echo -e "${YELLOW}8)${RESET} Limpiar compilación"
    echo -e "${YELLOW}9)${RESET} Verificar estado del sistema (SHM/Semáforos)"
    echo -e "${YELLOW}10)${RESET} Demostración anti busy-wait (watch + strace)"
    echo -e "${YELLOW}11)${RESET} Instalación completa"
    echo -e "${YELLOW}12)${RESET} Test rápido"
    echo -e "${YELLOW}13)${RESET} Salir"
    echo ""
    echo -n "Opción: "
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
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
            echo -e "${YELLOW}→ Instalando herramientas...${RESET}"
            sudo apt-get install -y build-essential gcc make
            sudo apt-get install -y libc6-dev
            sudo apt-get install -y gdb valgrind
            sudo apt-get install -y util-linux >/dev/null 2>&1 || true
            ;;
        redhat)
            echo -e "${YELLOW}→ Instalando grupo de desarrollo...${RESET}"
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y glibc-devel gdb valgrind
            ;;
        arch)
            echo -e "${YELLOW}→ Actualizando sistema...${RESET}"
            sudo pacman -Syu --noconfirm
            echo -e "${YELLOW}→ Instalando herramientas...${RESET}"
            sudo pacman -S --needed --noconfirm base-devel gcc make gdb valgrind
            ;;
        *)
            echo -e "${RED}⚠ Sistema no soportado automáticamente${RESET}"
            echo -e "${YELLOW}Instale manualmente: GCC, Make, pthread, librt${RESET}"
            return 1
            ;;
    esac
    
    echo ""
    echo -e "${GREEN}✓ Dependencias instaladas${RESET}"
}

build_project() {
    echo -e "${BOLD}${BLUE}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${BLUE}║                  COMPILACIÓN DEL RECEPTOR                  ║${RESET}"
    echo -e "${BOLD}${BLUE}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    if ! command_exists gcc || ! command_exists make; then
        echo -e "${RED}⚠ GCC o Make no están instalados${RESET}"
        echo -e "${YELLOW}Ejecute la opción 1 para instalar dependencias${RESET}"
        return 1
    fi
    
    echo -e "${YELLOW}→ Limpiando...${RESET}"
    make clean
    
    echo -e "${YELLOW}→ Compilando...${RESET}"
    if make; then
        echo ""
        echo -e "${GREEN}✓ Receptor compilado: bin/receptor${RESET}"
    else
        echo ""
        echo -e "${RED}✗ Error de compilación${RESET}"
        return 1
    fi
}

run_auto() {
    if [ ! -f "bin/receptor" ]; then
        echo -e "${RED}✗ Receptor no compilado${RESET}"
        echo -e "${YELLOW}Compilando primero...${RESET}"
        build_project || return 1
    fi
    
    echo -e "${YELLOW}Clave personalizada (hex 2 chars, Enter=usar del sistema):${RESET}"
    read -r key
    echo -e "${YELLOW}Delay (ms, Enter=100):${RESET}"
    read -r delay
    
    if [ -z "$key" ] && [ -z "$delay" ]; then
        ./bin/receptor auto
    elif [ -z "$delay" ]; then
        ./bin/receptor auto "$key"
    else
        ./bin/receptor auto "$key" "$delay"
    fi
}

run_manual() {
    if [ ! -f "bin/receptor" ]; then
        echo -e "${RED}✗ Receptor no compilado${RESET}"
        build_project || return 1
    fi
    
    echo -e "${YELLOW}Clave personalizada (hex 2 chars, Enter=usar del sistema):${RESET}"
    read -r key
    
    if [ -z "$key" ]; then
        ./bin/receptor manual
    else
        ./bin/receptor manual "$key"
    fi
}

launch_multiple() {
    if [ ! -f "bin/receptor" ]; then
        echo -e "${RED}✗ Receptor no compilado${RESET}"
        build_project || return 1
    fi
    
    echo -e "${YELLOW}¿Cuántos receptores lanzar?${RESET}"
    read -r num
    if ! [[ "$num" =~ ^[0-9]+$ ]] || [ "$num" -lt 1 ]; then
        echo -e "${RED}Número inválido${RESET}"
        return 1
    fi
    
    echo -e "${CYAN}→ Lanzando $num receptores...${RESET}"
    for i in $(seq 1 "$num"); do
        ./bin/receptor auto >/dev/null 2>&1 &
        echo -e "  ${GREEN}✓${RESET} Receptor $i iniciado (PID $!)"
        sleep 0.2
    done
    echo ""
    echo -e "${GREEN}✓ $num receptores activos${RESET}"
}

show_active() {
    echo -e "${BOLD}${CYAN}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${CYAN}║                  RECEPTORES ACTIVOS                        ║${RESET}"
    echo -e "${BOLD}${CYAN}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    pgrep receptor | xargs -r -I{} ps -o pid,stat,pcpu,wchan,cmd -p {}
}

kill_all() {
    echo -e "${BOLD}${RED}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${RED}║              TERMINANDO RECEPTORES (SIGUSR1)               ║${RESET}"
    echo -e "${BOLD}${RED}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    pgrep receptor | xargs -r -n1 -I{} kill -USR1 {}
    echo -e "${GREEN}✓ Señal enviada${RESET}"
}

clean_project() {
    echo -e "${YELLOW}→ Limpiando compilación...${RESET}"
    make clean
    echo -e "${GREEN}✓ Limpieza completada${RESET}"
}

check_system() {
    echo -e "${BOLD}${CYAN}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${CYAN}║              ESTADO DEL SISTEMA IPC                        ║${RESET}"
    echo -e "${BOLD}${CYAN}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    echo -e "${YELLOW}→ Memoria compartida:${RESET}"
    ipcs -m | grep -E "0x00001234|key" || echo "  No encontrada - ejecute el inicializador"
    
    echo ""
    echo -e "${YELLOW}→ Semáforos POSIX:${RESET}"
    ls -l /dev/shm/sem.* 2>/dev/null || echo "  No encontrados - ejecute el inicializador"
    
    echo ""
    echo -e "${YELLOW}→ Receptores activos:${RESET}"
    pgrep -c receptor 2>/dev/null || echo "  0"
}

demo_no_busy_wait() {
    echo -e "${BOLD}${MAGENTA}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${MAGENTA}║             DEMO: SIN BUSY WAITING                         ║${RESET}"
    echo -e "${BOLD}${MAGENTA}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    echo -e "${CYAN}1) Vista en vivo de PID/STAT/CPU/wchan (Ctrl+C para salir)${RESET}"
    echo -e "${YELLOW}   => Los receptores deben verse con wchan=futex, do_nanosleep o similares${RESET}"
    echo ""
    watch -n 0.5 'pgrep receptor | xargs -r -I{} ps -o pid,stat,pcpu,wchan,cmd -p {}'
    echo ""
    echo -e "${CYAN}2) Adjuntar strace a un PID (muestra futex/ppoll/nanosleep)${RESET}"
    echo -n "PID: "
    read -r PID
    if [[ "$PID" =~ ^[0-9]+$ ]]; then
        sudo strace -tt -p "$PID" -e trace=futex,ppoll,select,clock_nanosleep,nanosleep
    else
        echo -e "${RED}PID inválido${RESET}"
    fi
}

full_install() {
    install_dependencies || return 1
    echo ""
    build_project || return 1
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${GREEN}║            INSTALACIÓN COMPLETADA                          ║${RESET}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${RESET}"
}

quick_test() {
    echo -e "${BOLD}${MAGENTA}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}${MAGENTA}║                    TEST RÁPIDO                             ║${RESET}"
    echo -e "${BOLD}${MAGENTA}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    
    if [ ! -f "bin/receptor" ]; then
        echo -e "${YELLOW}→ Compilando...${RESET}"
        build_project || return 1
    fi
    
    echo -e "${CYAN}→ Verificando memoria compartida...${RESET}"
    if ipcs -m | grep 0x1234 > /dev/null; then
        echo -e "${GREEN}✓ Memoria compartida OK${RESET}"
    else
        echo -e "${RED}✗ Ejecute el inicializador primero${RESET}"
        return 1
    fi
    
    echo -e "${CYAN}→ Verificando semáforos...${RESET}"
    if ls /dev/shm/sem.* 2>/dev/null > /dev/null; then
        echo -e "${GREEN}✓ Semáforos OK${RESET}"
    else
        echo -e "${RED}✗ Ejecute el inicializador primero${RESET}"
        return 1
    fi
    
    echo -e "${CYAN}→ Lanzando receptor de prueba (5 seg, auto)...${RESET}"
    timeout 5 ./bin/receptor auto 2>/dev/null &
    PID=$!
    sleep 5
    kill $PID 2>/dev/null || true
    
    echo ""
    echo -e "${GREEN}✓ Test completado${RESET}"
}

main() {
    while true; do
        show_banner
        show_menu
        read -r option
        
        case $option in
            1) install_dependencies ;;
            2) build_project ;;
            3) run_auto ;;
            4) run_manual ;;
            5) launch_multiple ;;
            6) show_active ;;
            7) kill_all ;;
            8) clean_project ;;
            9) check_system ;;
            10) demo_no_busy_wait ;;
            11) full_install ;;
            12) quick_test ;;
            13) echo ""; echo -e "${CYAN}¡Hasta luego!${RESET}"; exit 0 ;;
            *) echo -e "${RED}Opción inválida${RESET}" ;;
        esac
        
        echo ""
        echo -e "${YELLOW}Presione Enter para continuar...${RESET}"
        read -r
    done
}

chmod +x "$0"
main