#include "common.h"

#define MAX_CLIENTS 20

SharedMemory *shm_ptr;
pthread_mutex_t shm_lock = PTHREAD_MUTEX_INITIALIZER;

void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);
    
    char buffer[1024];
    int bytes_read;
    
    printf("[SERVER] New client connected (Socket: %d)\n", client_sock);
    
    while((bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        
        // Parse and display the received data
       // char type[20];
        char name[50];
        char status[100];
        int id, alarm;
        float value;
        long timestamp;
        
        if (sscanf(buffer, "SENSOR,%d,%[^,],%f,%ld", &id, name, &value, &timestamp) == 4) {
            printf("[SERVER] SENSOR DATA | ID: %d | Type: %s | Value: %.2f | Time: %ld\n",
                   id, name, value, timestamp);
            
            // Update shared memory
            pthread_mutex_lock(&shm_lock);
            if (id >= 0 && id < NUM_SENSORS) {
                shm_ptr->sensors[id].value = value;
                strncpy(shm_ptr->sensors[id].name, name, sizeof(shm_ptr->sensors[id].name) - 1);
                shm_ptr->sensors[id].timestamp = timestamp;
            }
            pthread_mutex_unlock(&shm_lock);
        }
        else if (sscanf(buffer, "ACTUATOR,%d,%[^,],%f,%[^,],%d,%ld", 
                        &id, name, &value, status, &alarm, &timestamp) == 6) {
            printf("[SERVER] ACTUATOR DATA | ID: %d | Type: %s | Value: %.2f%% | Status: %s | Alarm: %s | Time: %ld\n",
                   id, name, value, status, alarm ? "ON" : "OFF", timestamp);
            
            // Update shared memory
            pthread_mutex_lock(&shm_lock);
            if (id >= 0 && id < NUM_SENSORS) {
                shm_ptr->actuators[id].value = value;
                strncpy(shm_ptr->actuators[id].name, name, sizeof(shm_ptr->actuators[id].name) - 1);
                strncpy(shm_ptr->actuators[id].status, status, sizeof(shm_ptr->actuators[id].status) - 1);
                shm_ptr->actuators[id].alarm_status = alarm;
                shm_ptr->actuators[id].timestamp = timestamp;
            }
            pthread_mutex_unlock(&shm_lock);
        }
    }
    
    printf("[SERVER] Client disconnected (Socket: %d)\n", client_sock);
    close(client_sock);
    pthread_exit(NULL);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t client_threads[MAX_CLIENTS];
    int thread_count = 0;
    
    printf("[SERVER] Starting initialization...\n");
    
    // ============ CREATE SHARED MEMORY ============
    int shm_id = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("[SERVER] Shared memory creation failed");
        exit(1);
    }
    printf("[SERVER] ✓ Shared memory created (ID: %d, Key: 0x%x)\n", shm_id, SHM_KEY);
    
    shm_ptr = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void*)-1) {
        perror("[SERVER] Shared memory attach failed");
        exit(1);
    }
    printf("[SERVER] ✓ Shared memory attached\n");
    
    // Initialize shared memory
    memset(shm_ptr, 0, sizeof(SharedMemory));
    printf("[SERVER] ✓ Shared memory initialized\n");
    // ============================================
    
    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(1);
    }
    
    // Bind socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // Listen for connections
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║         IoT SMART HOME SERVER                                 ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("[SERVER] ✓ Server started on port %d\n", SERVER_PORT);
    printf("[SERVER] ✓ Waiting for connections...\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    while(1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Create thread to handle client
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_sock;
        
        if (pthread_create(&client_threads[thread_count % MAX_CLIENTS], NULL, 
                          handle_client, client_sock_ptr) != 0) {
            perror("Thread creation failed");
            close(client_sock);
            free(client_sock_ptr);
            continue;
        }
        
        pthread_detach(client_threads[thread_count % MAX_CLIENTS]);
        thread_count++;
    }
    
    // Cleanup
    shmdt(shm_ptr);
    close(server_sock);
    return 0;
}
