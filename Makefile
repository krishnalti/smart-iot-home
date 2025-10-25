CC = gcc
CFLAGS = -Wall -pthread
NCURSES = -lncurses

all: server dcp dpcp ui

server: server.c
	$(CC) $(CFLAGS) -o server server.c

dcp: dcp.c
	$(CC) $(CFLAGS) -o dcp dcp.c

dpcp: dpcp.c
	$(CC) $(CFLAGS) -o dpcp dpcp.c

ui_alt: ui_alt.c
	$(CC) $(CFLAGS) -o ui ui.c

clean:
	rm -f server dcp dpcp ui
	ipcrm -a

run: all
	@echo "Starting Smart Home System..."
	@echo "Run in separate terminals:"
	@echo "  Terminal 1: ./server"
	@echo "  Terminal 2: ./dpcp"
	@echo "  Terminal 3: ./ui"
	@echo "  Terminal 4: ./dcp"

.PHONY: all clean run
