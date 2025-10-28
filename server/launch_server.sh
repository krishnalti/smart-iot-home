#!/bin/bash

# SERVER-SIDE LAUNCHER
# Runs: dcp_server, dpcp, ui, web_ui

echo "=========================================="
echo " IoT Smart Home - SERVER MACHINE"
echo "=========================================="

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

# Trap for cleanup
trap "echo -e '\n${RED}[CLEANUP]${NC} Shutting down server...'; kill 0" EXIT
trap "exit" INT TERM ERR

mkdir -p logs

# Clean shared memory
echo -e "${BLUE}[SETUP]${NC} Cleaning shared memory..."
ipcrm -M 1234 2>/dev/null
ipcrm -M 5678 2>/dev/null
rm -f /dev/shm/sem.sem_* 2>/dev/null

# Check executables
REQUIRED=("dcp_server" "dpcp" "ui" "web_ui")
MISSING=0

for exe in "${REQUIRED[@]}"; do
    if [ ! -f "./$exe" ]; then
        echo -e "${RED}[ERROR]${NC} Missing: $exe"
        MISSING=1
    fi
done

if [ $MISSING -eq 1 ]; then
    echo -e "${RED}[ERROR]${NC} Run 'make server' first"
    exit 1
fi

# Get server IP
SERVER_IP=$(hostname -I | awk '{print $1}')
echo ""
echo -e "${GREEN}[INFO]${NC} Server IP Address: ${YELLOW}$SERVER_IP${NC}"
echo -e "${GREEN}[INFO]${NC} Clients should use this IP in common.h"
echo ""

# Set capability for port 80
echo -e "${BLUE}[SETUP]${NC} Setting port 80 capability..."
if sudo setcap 'cap_net_bind_service=+ep' ./web_ui 2>/dev/null; then
    echo -e "${GREEN}[OK]${NC} Port 80 capability set"
else
    echo -e "${YELLOW}[WARNING]${NC} Could not set port 80 capability"
fi
echo ""

# Start DCP Server
echo -e "${GREEN}[START]${NC} Starting DCP Server (Port 8080)..."
gnome-terminal --title="DCP Server" --geometry=80x20+0+0 -- bash -c "
trap 'exit' INT TERM;
echo '========================================';
echo ' DCP SERVER';
echo ' Port: 8080';
echo ' Listening for client connections...';
echo '========================================';
echo '';
./dcp_server;
echo 'DCP Server stopped.';
read -t 3
" &
sleep 2

# Start DPCP
echo -e "${GREEN}[START]${NC} Starting DPCP (Control Processor)..."
gnome-terminal --title="DPCP" --geometry=80x20+850+0 -- bash -c "
trap 'exit' INT TERM;
echo '========================================';
echo ' DPCP (Control Processor)';
echo '========================================';
echo '';
./dpcp;
echo 'DPCP stopped.';
read -t 3
" &
sleep 1

# Start Web UI
echo -e "${GREEN}[START]${NC} Starting Web UI (Port 80)..."
gnome-terminal --title="Web UI" --geometry=80x15+0+400 -- bash -c "
trap 'exit' INT TERM;
echo '========================================';
echo ' WEB UI (HTTP Server)';
echo ' Port: 80';
echo ' URL: http://$SERVER_IP';
echo '========================================';
echo '';
./web_ui;
echo 'Web UI stopped.';
read -t 3
" &
sleep 1

# Start ncurses Dashboard
echo -e "${GREEN}[START]${NC} Starting ncurses Dashboard..."
gnome-terminal --title="Dashboard" --geometry=120x30+850+400 -- bash -c "
trap 'exit' INT TERM;
echo '========================================';
echo ' NCURSES DASHBOARD';
echo ' Press Ctrl+C to exit';
echo '========================================';
echo '';
./ui;
echo 'Dashboard stopped.';
read -t 3
" &
sleep 1

echo ""
echo "=========================================="
echo -e "${GREEN}SERVER RUNNING${NC}"
echo "=========================================="
echo "Server IP:        $SERVER_IP"
echo "DCP Server Port:  8080"
echo "Web Dashboard:    http://$SERVER_IP"
echo ""
echo "Components Running:"
echo "  ✓ DCP Server (receiving client data)"
echo "  ✓ DPCP (processing control logic)"
echo "  ✓ Web UI (HTTP dashboard)"
echo "  ✓ ncurses UI (terminal dashboard)"
echo ""
echo -e "${YELLOW}Waiting for clients to connect...${NC}"
echo ""
echo -e "${RED}Press Ctrl+C to stop all server processes${NC}"
echo "=========================================="

wait

