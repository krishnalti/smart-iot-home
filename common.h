#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// Configuration
#define NUM_ROOMS 2
#define NUM_SENSORS 6
#define DCP_SERVER_PORT 8080
#define WEB_SERVER_PORT 80

#define SERVER_IP "192.168.1.138"
#define UPDATE_INTERVAL 5
#define SHM_SENSOR_KEY 1234
#define SHM_CONTROL_KEY 5678

typedef enum {
    TEMP_SENSOR = 0,
    SMOKE_SENSOR,
    MOTION_SENSOR,
    DOOR_LOCK,
    HUMIDITY_SENSOR,
    LIGHT_SENSOR
} SensorType;

typedef struct {
    SensorType type;
    int device_id;
    int room_id;
    float value;
    char door_lock_id[32];
    long timestamp;
} SensorData;

typedef struct {
    int device_id;
    int room_id;
    char control_name[64];
    char action[128];
    char status[64];
    int alarm_status;
    long timestamp;
} ControlData;

typedef struct {
    pthread_mutex_t mutex;
    SensorData sensors[NUM_ROOMS][NUM_SENSORS];
    int batch_count;
} SensorSharedMemory;

typedef struct {
    pthread_mutex_t mutex;
    SensorData sensors[NUM_ROOMS][NUM_SENSORS];
    ControlData controls[NUM_ROOMS][NUM_SENSORS];
    int batch_count;
} ControlSharedMemory;

#endif

