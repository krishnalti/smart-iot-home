# IoT Smart Home System Makefile

CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -pthread
NCURSES_FLAG = -lncurses

# Target executables
TARGETS = dcp_client dcp_server dpcp ui web_ui

# Source files
CLIENT_SRC = dcp_client.c
SERVER_SRC = dcp_server.c
DPCP_SRC = dpcp.c
UI_SRC = ui.c
WEB_UI_SRC = web_ui.c

HEADERS = common.h

# Build all components
all: $(TARGETS)
	@echo "=================================="
	@echo "Build Complete!"
	@echo "=================================="
	@echo "Executables created:"
	@echo "  - dcp_client (IoT device simulator)"
	@echo "  - dcp_server (TCP server)"
	@echo "  - dpcp (Control processor)"
	@echo "  - ui (ncurses dashboard)"
	@echo "  - web_ui (HTTP server)"
	@echo ""
	@echo "For distributed setup:"
	@echo "  Server machine: make server"
	@echo "  Client machine: make client"
	@echo "=================================="

# Build ONLY client components (for client machine)
client: dcp_client
	@echo "=================================="
	@echo "CLIENT Build Complete!"
	@echo "=================================="
	@echo "Built: dcp_client"
	@echo ""
	@echo "Make sure SERVER_IP in common.h points to server machine"
	@echo "Run: ./launch_client.sh"
	@echo "=================================="

# Build ONLY server components (for server machine)
server: dcp_server dpcp ui web_ui
	@echo "=================================="
	@echo "SERVER Build Complete!"
	@echo "=================================="
	@echo "Built: dcp_server, dpcp, ui, web_ui"
	@echo ""
	@echo "Run: make setcap (for port 80)"
	@echo "Then: ./launch_server.sh"
	@echo "=================================="

# Individual builds
dcp_client: $(CLIENT_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o dcp_client $(CLIENT_SRC) $(LDFLAGS)
	@echo "[✓] dcp_client compiled"

dcp_server: $(SERVER_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o dcp_server $(SERVER_SRC) $(LDFLAGS)
	@echo "[✓] dcp_server compiled"

dpcp: $(DPCP_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o dpcp $(DPCP_SRC) $(LDFLAGS)
	@echo "[✓] dpcp compiled"

ui: $(UI_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o ui $(UI_SRC) $(LDFLAGS) $(NCURSES_FLAG)
	@echo "[✓] ui compiled"

web_ui: $(WEB_UI_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o web_ui $(WEB_UI_SRC) $(LDFLAGS)
	@echo "[✓] web_ui compiled"

# Set port 80 capability (SERVER machine only)
setcap: web_ui
	@echo "=================================="
	@echo "Setting port 80 capability..."
	@echo "=================================="
	@sudo setcap 'cap_net_bind_service=+ep' ./web_ui
	@getcap ./web_ui
	@echo "[✓] web_ui can bind to port 80"
	@echo "=================================="

# Clean
clean:
	rm -f $(TARGETS) *.o
	@echo "[✓] Clean complete"

# Clean shared memory (SERVER machine only)
clean-shm:
	@echo "Cleaning shared memory..."
	@ipcrm -M 1234 2>/dev/null || true
	@ipcrm -M 5678 2>/dev/null || true
	@rm -f /dev/shm/sem.sem_* 2>/dev/null || true
	@echo "[✓] Shared memory cleaned"

# Install dependencies
install-deps:
	@echo "Installing dependencies..."
	sudo apt-get update
	sudo apt-get install -y build-essential libncurses5-dev libncursesw5-dev libcap2-bin
	@echo "[✓] Dependencies installed"

# Help
help:
	@echo "IoT Smart Home System - Makefile"
	@echo "=================================="
	@echo "make all         - Build all components"
	@echo "make server      - Build server components only"
	@echo "make client      - Build client components only"
	@echo "make setcap      - Set port 80 capability (server)"
	@echo "make clean       - Remove binaries"
	@echo "make clean-shm   - Clean shared memory (server)"
	@echo "make install-deps- Install dependencies"
	@echo ""
	@echo "Distributed Setup:"
	@echo "  1. Update SERVER_IP in common.h"
	@echo "  2. Server: make server && make setcap && ./launch_server.sh"
	@echo "  3. Client: make client && ./launch_client.sh"

.PHONY: all client server clean clean-shm setcap install-deps help

