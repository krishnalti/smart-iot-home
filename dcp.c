#include "common.h"

SharedMemory *shm_ptr;
sem_t *sem_dcp;//used for sync
sem_t *sem_dpcp;

const char* sensor_names[] = {
    "Temperature Sensor",
    "Humidity Sensor",
    "Light Sensor",
    "Gas Sensor",
    "Motion Sensor"
};

// Function to generate random sensor values based on type
float generate_sensor_value(SensorType type) {
    switch(type) {
        case TEMP_SENSOR:
            return 15.0 + (rand() % 30);  // 15-45°C
      //  case HUMIDITY_SENSOR:
        //    return 30.0 + (rand() % 60);  // 30-90%
        case LIGHT_SENSOR:
            return (rand() % 1000);  // 0-1000 lux
        case GAS_SENSOR:
            return (rand() % 100);  // 0-100 ppm
        case MOTION_SENSOR:
            return (rand() % 2);  // 0 or 1
        default:
            return 0.0;
    }
}

// Sensor thread function
void* sensor_thread_func(void* arg) {
    int sensor_id = *(int*)arg;
    free(arg);
    
    int sock;
    struct sockaddr_in server_addr;
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sock);
        pthread_exit(NULL);
    }
    
    printf("[DCP] Sensor %d connected to server\n", sensor_id);
    
    while(1) {
        // Generate random sensor value
        float value = generate_sensor_value((SensorType)sensor_id);
        
        // Write to shared memory
        shm_ptr->sensors[sensor_id].type = (SensorType)sensor_id;
        shm_ptr->sensors[sensor_id].id = sensor_id;
        shm_ptr->sensors[sensor_id].value = value;
        shm_ptr->sensors[sensor_id].timestamp = time(NULL);
        strcpy(shm_ptr->sensors[sensor_id].name, sensor_names[sensor_id]);
        
        // Send data to server
        char buffer[256];
        snprintf(buffer, sizeof(buffer), 
                "SENSOR,%d,%s,%.2f,%ld\n",
                sensor_id, sensor_names[sensor_id], value, time(NULL));
        
        send(sock, buffer, strlen(buffer), 0);
        
        printf("[DCP] Sensor %d (%s): %.2f\n", 
               sensor_id, sensor_names[sensor_id], value);
        
        sleep(UPDATE_INTERVAL);
    }
    
    close(sock);
    pthread_exit(NULL);
}

int main() {
    pthread_t sensor_threads[NUM_SENSORS];
    
    // Initialize random seed
    srand(time(NULL));
    
    // Create/Get shared memory
    int shm_id = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("Shared memory creation failed");
        exit(1);
    }
    
    shm_ptr = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void*)-1) {
        perror("Shared memory attach failed");
        exit(1);
    }
    
    // Initialize shared memory
    memset(shm_ptr, 0, sizeof(SharedMemory));
    shm_ptr->initialized = 0;
    
    // Create named semaphores
    sem_unlink(SEM_DCP);
    sem_unlink(SEM_DPCP);
    
    sem_dcp = sem_open(SEM_DCP, O_CREAT, 0644, 0);
    sem_dpcp = sem_open(SEM_DPCP, O_CREAT, 0644, 0);
    
    if (sem_dcp == SEM_FAILED || sem_dpcp == SEM_FAILED) {
        perror("Semaphore creation failed");
        exit(1);
    }
    
    printf("[DCP] Data Communication Process started\n");
    printf("[DCP] Creating %d sensor threads...\n", NUM_SENSORS);
    
    // Create sensor threads
    for (int i = 0; i < NUM_SENSORS; i++) {
        int *sensor_id = malloc(sizeof(int));
        *sensor_id = i;
        
        if (pthread_create(&sensor_threads[i], NULL, sensor_thread_func, sensor_id) != 0) {
            perror("Thread creation failed");
            exit(1);
        }
    }
    
    // Wait for first batch of data generation
    sleep(UPDATE_INTERVAL + 1);
    
    shm_ptr->initialized = 1;
    printf("[DCP] First batch of sensor data generated\n");
    
    // Signal DPCP to start
    sem_post(sem_dcp);
    printf("[DCP] Signaled DPCP to start\n");
    
    // Wait for all sensor threads (they run indefinitely)
    for (int i = 0; i < NUM_SENSORS; i++) {
        pthread_join(sensor_threads[i], NULL);
    }
    
    // Cleanup
    shmdt(shm_ptr);
    sem_close(sem_dcp);
    sem_close(sem_dpcp);
    
    return 0;
}
