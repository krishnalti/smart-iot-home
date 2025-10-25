#include "common.h"

SharedMemory *shm_ptr;
sem_t *sem_dcp, *sem_dpcp, *sem_ui;

const char* actuator_names[] = {
    "AC/Fan",
    "Humidifier",
    "Smart Light",
    "Exhaust Fan",
    "Security Alarm"
};

Thresholds thresholds = {
    .temp_max = 35.0,      // Temperature > 35°C
    .humidity_max = 70.0,   // Humidity > 70%
    .light_max = 800.0,     // Light > 800 lux
    .gas_max = 50.0,        // Gas > 50 ppm
    .motion_detected = 1    // Motion detected
};

// Function to process sensor data and control actuator
void process_actuator_action(int actuator_id, float sensor_value, ActuatorData *actuator) {
    actuator->type = (ActuatorType)actuator_id;
    actuator->id = actuator_id;
    actuator->timestamp = time(NULL);
    strcpy(actuator->name, actuator_names[actuator_id]);
    
    switch(actuator_id) {
        case AC_FAN:
            if (sensor_value > thresholds.temp_max) {
                actuator->value = 100.0;  // Full speed
                actuator->alarm_status = 1;
                strcpy(actuator->status, "COOLING - HIGH TEMP ALARM!");
            } else if (sensor_value > 28.0) {
                actuator->value = 60.0;
                actuator->alarm_status = 0;
                strcpy(actuator->status, "COOLING");
            } else {
                actuator->value = 0.0;
                actuator->alarm_status = 0;
                strcpy(actuator->status, "OFF");
            }
            break;
            
      /*  case HUMIDIFIER:
            if (sensor_value > thresholds.humidity_max) {
                actuator->value = 100.0;
                actuator->alarm_status = 1;
                strcpy(actuator->status, "DEHUMIDIFYING - HIGH HUMIDITY ALARM!");
            } else if (sensor_value < 40.0) {
                actuator->value = 70.0;
                actuator->alarm_status = 0;
                strcpy(actuator->status, "HUMIDIFYING");
            } else {
                actuator->value = 0.0;
                actuator->alarm_status = 0;
                strcpy(actuator->status, "OFF");
            }
            break;
      */      
        case SMART_LIGHT:
            if (sensor_value > thresholds.light_max) {
                actuator->value = 0.0;
                actuator->alarm_status = 0;
                strcpy(actuator->status, "OFF - Bright");
            } else if (sensor_value < 200.0) {
                actuator->value = 100.0;
                actuator->alarm_status = 0;
                strcpy(actuator->status, "ON - Full Brightness");
            } else {
                actuator->value = 50.0;
                actuator->alarm_status = 0;
                strcpy(actuator->status, "ON - Medium");
            }
            break;
            
        case EXHAUST_FAN:
            if (sensor_value > thresholds.gas_max) {
                actuator->value = 100.0;
                actuator->alarm_status = 1;
                strcpy(actuator->status, "EMERGENCY - GAS LEAK DETECTED!");
            } else if (sensor_value > 30.0) {
                actuator->value = 60.0;
                actuator->alarm_status = 0;
                strcpy(actuator->status, "VENTILATING");
            } else {
                actuator->value = 0.0;
                actuator->alarm_status = 0;
                strcpy(actuator->status, "OFF");
            }
            break;
            
        case SECURITY_ALARM:
            // FIXED: Explicitly handle both cases
            if (sensor_value > 0.0) {
                // Motion detected
                actuator->value = 1.0;
                actuator->alarm_status = 1;
                strcpy(actuator->status, "ALERT - MOTION DETECTED!");
                printf("[DPCP] Motion Sensor: DETECTED (value=%.2f)\n", sensor_value);
            } else {
                // No motion (sensor_value == 0.0)
                actuator->value = 0.0;
                actuator->alarm_status = 0;
                strcpy(actuator->status, "ARMED - No Motion");
                printf("[DPCP] Motion Sensor: NO MOTION (value=%.2f)\n", sensor_value);
            }
            break;
    }
}

// Actuator thread function
void* actuator_thread_func(void* arg) {
    int actuator_id = *(int*)arg;
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
    
    printf("[DPCP] Actuator %d (%s) connected to server\n", actuator_id, actuator_names[actuator_id]);
    
    while(1) {
        // Read sensor data from shared memory
        float sensor_value = shm_ptr->sensors[actuator_id].value;
        
        // Debug: Print sensor value being read
        if (actuator_id == MOTION_SENSOR) {
            printf("[DPCP] Reading Motion Sensor value from shared memory: %.2f\n", sensor_value);
        }
        
        // Process and determine actuator action
        ActuatorData actuator_data;
        process_actuator_action(actuator_id, sensor_value, &actuator_data);
        
        // Write actuator data to shared memory
        shm_ptr->actuators[actuator_id] = actuator_data;
        
        // Send actuator data to server
        char buffer[512];
        snprintf(buffer, sizeof(buffer), 
                "ACTUATOR,%d,%s,%.2f,%s,%d,%ld\n",
                actuator_id, actuator_names[actuator_id], 
                actuator_data.value, actuator_data.status,
                actuator_data.alarm_status, time(NULL));
        
        send(sock, buffer, strlen(buffer), 0);
        
        printf("[DPCP] Actuator %d (%s): %.2f%% - %s %s\n", 
               actuator_id, actuator_names[actuator_id], 
               actuator_data.value, actuator_data.status,
               actuator_data.alarm_status ? "[ALARM ON]" : "");
        
        sleep(UPDATE_INTERVAL);
    }
    
    close(sock);
    pthread_exit(NULL);
}

int main() {
    pthread_t actuator_threads[NUM_SENSORS];
    
    // Get shared memory
    int shm_id = shmget(SHM_KEY, sizeof(SharedMemory), 0666);
    if (shm_id < 0) {
        perror("Shared memory access failed");
        exit(1);
    }
    
    shm_ptr = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void*)-1) {
        perror("Shared memory attach failed");
        exit(1);
    }
    
    // Open semaphores
    sem_dcp = sem_open(SEM_DCP, 0);
    sem_dpcp = sem_open(SEM_DPCP, 0);
    
    sem_unlink(SEM_UI);
    sem_ui = sem_open(SEM_UI, O_CREAT, 0644, 0);
    
    if (sem_dcp == SEM_FAILED || sem_dpcp == SEM_FAILED || sem_ui == SEM_FAILED) {
        perror("Semaphore open failed");
        exit(1);
    }
    
    printf("[DPCP] Data Processing and Control Process started\n");
    printf("[DPCP] Waiting for DCP to generate first batch of data...\n");
    
    // Wait for DCP signal
    sem_wait(sem_dcp);
    
    printf("[DPCP] Received signal from DCP. Starting actuator threads...\n");
    
    // Create actuator threads
    for (int i = 0; i < NUM_SENSORS; i++) {
        int *actuator_id = malloc(sizeof(int));
        *actuator_id = i;
        
        if (pthread_create(&actuator_threads[i], NULL, actuator_thread_func, actuator_id) != 0) {
            perror("Thread creation failed");
            exit(1);
        }
    }
    
    // Wait for first batch of actuator processing
    sleep(UPDATE_INTERVAL + 1);
    
    // Signal UI to start
    sem_post(sem_dpcp);
    printf("[DPCP] Signaled UI to start\n");
    
    // Wait for all actuator threads
    for (int i = 0; i < NUM_SENSORS; i++) {
        pthread_join(actuator_threads[i], NULL);
    }
    
    // Cleanup
    shmdt(shm_ptr);
    sem_close(sem_dcp);
    sem_close(sem_dpcp);
    sem_close(sem_ui);
    
    return 0;
}
