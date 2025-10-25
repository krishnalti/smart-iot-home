#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define NUM_SENSORS 5
#define SERVER_PORT 2025//from 8080->2025
#define SERVER_IP "127.0.0.1"
#define SHM_KEY 1234
#define UPDATE_INTERVAL 10  // seconds

// Named semaphore names
#define SEM_DCP "/sem_dcp"
#define SEM_DPCP "/sem_dpcp"
#define SEM_UI "/sem_ui"

typedef enum {
    TEMP_SENSOR = 0,
   // HUMIDITY_SENSOR,
    LIGHT_SENSOR,
    GAS_SENSOR,
    MOTION_SENSOR
} SensorType;

typedef enum {
    AC_FAN = 0,
    //HUMIDIFIER,
    SMART_LIGHT,
    EXHAUST_FAN,
    SECURITY_ALARM
} ActuatorType;

typedef struct {
    SensorType type;
    int id;
    float value;
    time_t timestamp;
    char name[50];
} SensorData;

typedef struct {
    ActuatorType type;
    int id;
    float value;
    int alarm_status;  // 0: OFF, 1: ON
    time_t timestamp;
    char name[50];
    char status[50];
} ActuatorData;

typedef struct {
    SensorData sensors[NUM_SENSORS];
    ActuatorData actuators[NUM_SENSORS];
    int initialized;
} SharedMemory;

// Threshold values for danger detection
typedef struct {
    float temp_max;
    float humidity_max;
    float light_max;
    float gas_max;
    int motion_detected;
} Thresholds;

#endif
