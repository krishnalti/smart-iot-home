#include "common.h"

volatile sig_atomic_t running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\n[CLIENT] Shutting down...\n");
        running = 0;
    }
}

float generate_sensor_value(SensorType type) {
    switch(type) {
        case TEMP_SENSOR: return 15.0 + (rand() % 35);
        case SMOKE_SENSOR: return rand() % 100;
        case MOTION_SENSOR: return rand() % 4;
        case DOOR_LOCK: return rand() % 2;
        case HUMIDITY_SENSOR: return 20.0 + (rand() % 70);
        case LIGHT_SENSOR: return rand() % 1000;
        default: return 0.0;
    }
}

typedef struct {
    int sensor_id;
    int room_id;
} SensorThreadArgs;

void *sensor_thread(void *arg) {
    SensorThreadArgs *args = (SensorThreadArgs*)arg;
    int sensor_id = args->sensor_id;
    int room_id = args->room_id;
    free(args);
    
    const char *sensor_names[] = {
        "Temperature Sensor", "Smoke Detector", "Motion Sensor",
        "Door Lock", "Humidity Sensor", "Light Sensor"
    };
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DCP_SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("[CLIENT] Room %d: Failed to connect sensor %d\n", room_id + 1, sensor_id);
        return NULL;
    }
    
    printf("[CLIENT] Room %d: %s connected\n", room_id + 1, sensor_names[sensor_id]);
    
    while(running) {
        float value = generate_sensor_value((SensorType)sensor_id);
        char door_id[32];
        
        if (sensor_id == DOOR_LOCK) {
            if (value > 0) {
                // Generate valid access codes for each room
                if (room_id == 0) {
                    sprintf(door_id, "VALID-A%d", (rand() % 5) + 1);  // Room1: A1-A5
                } else {
                    sprintf(door_id, "VALID-B%d", (rand() % 5) + 1);  // Room2: B1-B5
                }
            } else {
                sprintf(door_id, "INVALID");
            }
        } else {
            strcpy(door_id, "N/A");
        }
        
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%d,%d,%s,%.2f,%s,%ld",
                 room_id, sensor_id, sensor_names[sensor_id], 
                 value, door_id, time(NULL));
        
        send(sock, buffer, strlen(buffer), 0);
        sleep(UPDATE_INTERVAL);
    }
    
    close(sock);
    return NULL;
}

int main() {
    signal(SIGINT, signal_handler);
    srand(time(NULL));
    
    printf("===========================================\n");
    printf(" IoT Device Simulator - Multi-Room\n");
    printf("===========================================\n");
    printf("[CLIENT] Server: %s:%d\n", SERVER_IP, DCP_SERVER_PORT);
    printf("[CLIENT] Simulating %d rooms with %d sensors each\n", NUM_ROOMS, NUM_SENSORS);
    printf("===========================================\n\n");
    
    pthread_t threads[NUM_ROOMS * NUM_SENSORS];
    int thread_idx = 0;
    
    for (int room = 0; room < NUM_ROOMS; room++) {
        for (int sensor = 0; sensor < NUM_SENSORS; sensor++) {
            SensorThreadArgs *args = malloc(sizeof(SensorThreadArgs));
            args->sensor_id = sensor;
            args->room_id = room;
            pthread_create(&threads[thread_idx++], NULL, sensor_thread, args);
            usleep(100000);
        }
    }
    
    for (int i = 0; i < NUM_ROOMS * NUM_SENSORS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("[CLIENT] All sensors stopped\n");
    return 0;
}

