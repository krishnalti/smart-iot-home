#!/bin/bash

# CLIENT-SIDE LAUNCHER
# Runs: dcp_client only

echo "=========================================="
echo " IoT Smart Home - CLIENT MACHINE"
echo "=========================================="

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

# Trap for cleanup
trap "echo -e '\n${RED}[CLEANUP]${NC} Stopping client...'; kill 0" EXIT
trap "exit" INT TERM ERR

# Check if dcp_client exists
if [ ! -f "./dcp_client" ]; then
    echo -e "${RED}[ERROR]${NC} dcp_client not found"
    echo -e "${YELLOW}[INFO]${NC} Run 'make client' first"
    exit 1
fi

# Read SERVER_IP from common.h
SERVER_IP=$(grep '#define SERVER_IP' common.h | awk '{print $3}' | tr -d '"')

echo ""
echo -e "${BLUE}[INFO]${NC} Server IP from common.h: ${YELLOW}$SERVER_IP${NC}"
echo ""

# Prompt to change server IP if needed
read -p "Is this the correct server IP? (y/n): " confirm

if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
    read -p "Enter correct server IP address: " NEW_IP
    echo -e "${YELLOW}[NOTE]${NC} Update SERVER_IP in common.h to: \"$NEW_IP\""
    echo -e "${YELLOW}[NOTE]${NC} Then run 'make client' again"
    exit 1
fi

# Test connection
echo ""
echo -e "${BLUE}[TEST]${NC} Testing connection to server..."
if timeout 3 bash -c "</dev/tcp/$SERVER_IP/8080" 2>/dev/null; then
    echo -e "${GREEN}[OK]${NC} Server is reachable at $SERVER_IP:8080"
else
    echo -e "${RED}[ERROR]${NC} Cannot reach server at $SERVER_IP:8080"
    echo -e "${YELLOW}[INFO]${NC} Make sure:"
    echo "  1. Server is running"
    echo "  2. SERVER_IP in common.h is correct"
    echo "  3. Firewall allows port 8080"
    read -p "Continue anyway? (y/n): " proceed
    if [ "$proceed" != "y" ] && [ "$proceed" != "Y" ]; then
        exit 1
    fi
fi

echo ""
echo -e "${GREEN}[START]${NC} Starting IoT Device Simulators..."

# Start DCP Client
gnome-terminal --title="DCP Client - IoT Devices" --geometry=90x30+100+100 -- bash -c "
trap 'exit' INT TERM;
echo '========================================';
echo ' DCP CLIENT (IoT Device Simulators)';
echo ' Connecting to: $SERVER_IP:8080';
echo '========================================';
echo '';
./dcp_client;
echo '';
echo 'Client stopped.';
read -t 5
" &

echo ""
echo "=========================================="
echo -e "${GREEN}CLIENT RUNNING${NC}"
echo "=========================================="
echo "Server Address:   $SERVER_IP:8080"
echo "Simulating:       6 IoT Devices"
echo ""
echo "Devices:"
echo "  • Temperature Sensor"
echo "  • Smoke Detector"
echo "  • Motion Sensor"
echo "  • Door Lock"
echo "  • Humidity Sensor"
echo "  • Light Sensor"
echo ""
echo -e "${RED}Press Ctrl+C to stop client${NC}"
echo "=========================================="

wait

