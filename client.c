#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define PIPE_NAME "server_pid"

typedef struct {
    int room_id;
    int temperature;
    int motion;
    int smoke;
    int face_id;
} room;

pid_t server_pid;  // Global so signal handler can access

int generate_random(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nSIGINT received. Terminating client and notifying server (PID %d)...\n", server_pid);
        kill(server_pid, SIGUSR1);  // Send signal to server
        system("pkill -SIGINT dpcp");
        exit(0);  // Terminate client
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <room_id>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, signal_handler);  // Register signal handler
    srand(time(NULL));  // Seed random generator

    // Read server PID from named pipe
    int pid_fd = open(PIPE_NAME, O_RDONLY);
    if (pid_fd < 0) {
        perror("Failed to open server_pid pipe");
        return 1;
    }
    read(pid_fd, &server_pid, sizeof(server_pid));
    close(pid_fd);
    printf("Received server PID: %d\n", server_pid);

    // Create socket
    int sock;
    struct sockaddr_in server_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    int room_id = atoi(argv[1]);
    printf("Room %d connected to server.\n", room_id);

    // Send data loop
    while (1) {
        room data;
        data.room_id = room_id;
        data.temperature = generate_random(18, 40);
        data.motion = generate_random(0, 1);
        data.smoke = generate_random(0, 1);
        data.face_id = generate_random(1, 10);

        send(sock, &data, sizeof(data), 0);
        printf("Room %d sent data: Temp=%d, Motion=%d, Smoke=%d, FaceID=%d\n",
               data.room_id, data.temperature, data.motion, data.smoke, data.face_id);

        sleep(20);  // Wait 20 seconds
    }

    close(sock);
    return 0;
}

