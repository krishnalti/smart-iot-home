#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

clear
echo -e "${BOLD}${CYAN}"
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                                                               ║"
echo "║           🚀  SYSTEM LAUNCHER SCRIPT  🚀                      ║"
echo "║                                                               ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"
sleep 1

# Step 1: Clean and build
echo -e "${BOLD}${YELLOW}┌─────────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BOLD}${YELLOW}│  Step 1/5: Cleaning previous builds...                         │${NC}"
echo -e "${BOLD}${YELLOW}└─────────────────────────────────────────────────────────────────┘${NC}"
make clean
sleep 1
echo ""

echo -e "${BOLD}${YELLOW}┌─────────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BOLD}${YELLOW}│  Step 2/5: Building all components...                          │${NC}"
echo -e "${BOLD}${YELLOW}└─────────────────────────────────────────────────────────────────┘${NC}"
make
if [ $? -ne 0 ]; then
    echo -e "${BOLD}${RED}"
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo "║  ❌ BUILD FAILED! Please check for errors.                   ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
    exit 1
fi
echo -e "${BOLD}${GREEN}✓ Build successful!${NC}"
echo ""
sleep 1

# Check which terminal emulator is available
echo -e "${BOLD}${BLUE}┌─────────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BOLD}${BLUE}│  Detecting terminal emulator...                                 │${NC}"
echo -e "${BOLD}${BLUE}└─────────────────────────────────────────────────────────────────┘${NC}"
if command -v gnome-terminal &> /dev/null; then
    TERMINAL="gnome-terminal"
    TERM_CMD="--"
elif command -v xterm &> /dev/null; then
    TERMINAL="xterm"
    TERM_CMD="-e"
elif command -v konsole &> /dev/null; then
    TERMINAL="konsole"
    TERM_CMD="-e"
elif command -v xfce4-terminal &> /dev/null; then
    TERMINAL="xfce4-terminal"
    TERM_CMD="-e"
else
    echo -e "${BOLD}${RED}"
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo "║  ❌ ERROR: No suitable terminal emulator found!              ║"
    echo "║  Please install: gnome-terminal, xterm, konsole, or xfce4    ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Using terminal: ${BOLD}$TERMINAL${NC}"
echo ""
sleep 1

# Step 2: Launch Server
echo -e "${BOLD}${MAGENTA}┌─────────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BOLD}${MAGENTA}│  Step 3/5: 🖥️  Starting Server...                              │${NC}"
echo -e "${BOLD}${MAGENTA}└─────────────────────────────────────────────────────────────────┘${NC}"
$TERMINAL $TERM_CMD bash -c "./server; echo 'Server stopped. Press Enter to close...'; read" &
echo -e "${CYAN}⏳ Waiting for server to initialize shared memory...${NC}"
sleep 5
echo -e "${GREEN}✓ Server launched${NC}"
echo ""

# Step 3: Launch DPCP
echo -e "${BOLD}${MAGENTA}┌─────────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BOLD}${MAGENTA}│  Step 4/5: ⚙️  Starting DPCP Processor...                      │${NC}"
echo -e "${BOLD}${MAGENTA}└─────────────────────────────────────────────────────────────────┘${NC}"
$TERMINAL $TERM_CMD bash -c "./dpcp; echo 'DPCP stopped. Press Enter to close...'; read" &
sleep 3
echo -e "${GREEN}✓ DPCP launched${NC}"
echo ""

# Step 4: Launch UI
echo -e "${BOLD}${MAGENTA}┌─────────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BOLD}${MAGENTA}│  Step 5/5: 🎨 Starting UI Dashboard...                         │${NC}"
echo -e "${BOLD}${MAGENTA}└─────────────────────────────────────────────────────────────────┘${NC}"
$TERMINAL $TERM_CMD bash -c "./ui; echo 'UI stopped. Press Enter to close...'; read" &
sleep 3
echo -e "${GREEN}✓ UI Dashboard launched${NC}"
echo ""

# Step 5: Launch DCP (Data Collection Process)
echo -e "${BOLD}${MAGENTA}┌─────────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BOLD}${MAGENTA}│  📡 Starting DCP...                                             │${NC}"
echo -e "${BOLD}${MAGENTA}└─────────────────────────────────────────────────────────────────┘${NC}"
$TERMINAL $TERM_CMD bash -c "./dcp; echo 'DCP stopped. Press Enter to close...'; read" &
sleep 2
echo -e "${GREEN}✓ DCP launched${NC}"
echo ""

sleep 1
echo -e "${BOLD}${GREEN}"
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                                                               ║"
echo "║          ✅  ALL COMPONENTS LAUNCHED SUCCESSFULLY!  ✅        ║"
echo "║                                                               ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo ""
echo -e "${BOLD}${CYAN}📊 Active Terminals:${NC}"
echo -e "  ${GREEN}▶${NC} Terminal 1: ${BOLD}Server${NC}"
echo -e "  ${GREEN}▶${NC} Terminal 2: ${BOLD}DPCP Processor${NC}"
echo -e "  ${GREEN}▶${NC} Terminal 3: ${BOLD}UI Dashboard${NC}"
echo -e "  ${GREEN}▶${NC} Terminal 4: ${BOLD}DCP${NC}"
echo ""
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BOLD}${RED}⚠️  TO STOP ALL PROCESSES:${NC}"
echo -e "   • Press ${BOLD}Ctrl+C${NC} in each terminal window"
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
