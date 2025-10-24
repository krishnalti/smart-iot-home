#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/stat.h>

#define SERVER_PORT 8080
#define PIPE_NAME "server_pid"

typedef struct {
    int room_id;
    int temperature;
    int motion;
    int smoke;
    int face_id;
} room;

int server_socket;  // Global for cleanup

void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        printf("\nSIGUSR1 received. Shutting down server gracefully...\n");

        close(server_socket);       // Close socket
        unlink(PIPE_NAME);          // Remove named pipe
        exit(0);                    // Exit server
    }
}

int main() {
    signal(SIGUSR1, signal_handler);  // Register signal handler

    // Create named pipe and write server PID
    mkfifo(PIPE_NAME, 0666);
    int pid_fd = open(PIPE_NAME, O_WRONLY);
    pid_t server_pid = getpid();
    write(pid_fd, &server_pid, sizeof(server_pid));
    close(pid_fd);
    printf("Server PID (%d) written to pipe '%s'.\n", server_pid, PIPE_NAME);

    // Create socket
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    // Listen for connections
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    // Accept and handle clients
    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected.\n");

        while (1) {
            room data;
            int bytes_received = recv(client_socket, &data, sizeof(data), 0);
            if (bytes_received <= 0) {
                printf("Client disconnected.\n");
                close(client_socket);
                break;
            }

            printf("Received from Room %d: Temp=%d, Motion=%d, Smoke=%d, FaceID=%d\n",
                   data.room_id, data.temperature, data.motion, data.smoke, data.face_id);
        }
    }

    close(server_socket);
    unlink(PIPE_NAME);  // Cleanup pipe
    return 0;
}

