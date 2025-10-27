#include "common.h"

SensorSharedMemory *sensor_shm;
volatile sig_atomic_t running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\n[DCP SERVER] Shutting down...\n");
        running = 0;
    }
}

void *client_handler(void *arg) {
    int client_sock = *(int*)arg;
    free(arg);
    char buffer[256];
    
    while(running) {
        int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        
        buffer[bytes] = '\0';
        
        int room_id, sensor_id;
        char name[50], door_id[32];
        float value;
        long timestamp;
        
        sscanf(buffer, "%d,%d,%49[^,],%f,%31[^,],%ld",
               &room_id, &sensor_id, name, &value, door_id, &timestamp);
        
        pthread_mutex_lock(&sensor_shm->mutex);
        
        if (room_id < NUM_ROOMS && sensor_id < NUM_SENSORS) {
            sensor_shm->sensors[room_id][sensor_id].type = (SensorType)sensor_id;
            sensor_shm->sensors[room_id][sensor_id].device_id = sensor_id;
            sensor_shm->sensors[room_id][sensor_id].room_id = room_id;
            sensor_shm->sensors[room_id][sensor_id].value = value;
            strcpy(sensor_shm->sensors[room_id][sensor_id].door_lock_id, door_id);
            sensor_shm->sensors[room_id][sensor_id].timestamp = timestamp;
            sensor_shm->batch_count++;
        }
        
        pthread_mutex_unlock(&sensor_shm->mutex);
    }
    
    close(client_sock);
    return NULL;
}

int main() {
    signal(SIGINT, signal_handler);
    
    printf("===========================================\n");
    printf(" DCP Server - Multi-Room Support\n");
    printf("===========================================\n");
    
    int shm_id = shmget(SHM_SENSOR_KEY, sizeof(SensorSharedMemory), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("[DCP SERVER] shmget failed");
        exit(1);
    }
    
    sensor_shm = (SensorSharedMemory*)shmat(shm_id, NULL, 0);
    if (sensor_shm == (void*)-1) {
        perror("[DCP SERVER] shmat failed");
        exit(1);
    }
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&sensor_shm->mutex, &attr);
    sensor_shm->batch_count = 0;
    
    for (int r = 0; r < NUM_ROOMS; r++) {
        for (int s = 0; s < NUM_SENSORS; s++) {
            memset(&sensor_shm->sensors[r][s], 0, sizeof(SensorData));
            sensor_shm->sensors[r][s].room_id = r;
            sensor_shm->sensors[r][s].device_id = s;
        }
    }
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[DCP SERVER] Socket creation failed");
        exit(1);
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DCP_SERVER_PORT);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[DCP SERVER] Bind failed");
        exit(1);
    }
    
    listen(server_fd, 20);
    printf("[DCP SERVER] Listening on port %d\n", DCP_SERVER_PORT);
    printf("[DCP SERVER] Supporting %d rooms\n", NUM_ROOMS);
    printf("===========================================\n\n");
    
    while(running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int *client_sock = malloc(sizeof(int));
        
        *client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (*client_sock < 0) {
            free(client_sock);
            continue;
        }
        
        pthread_t thread;
        pthread_create(&thread, NULL, client_handler, client_sock);
        pthread_detach(thread);
    }
    
    close(server_fd);
    shmdt(sensor_shm);
    return 0;
}

