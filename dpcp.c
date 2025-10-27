#include "common.h"

SensorSharedMemory* sensor_shm;
ControlSharedMemory* control_shm;
volatile sig_atomic_t running = 1;

void signal_handler(int signum) 
{
    if (signum == SIGINT) 
    {
        printf("\n[DPCP] Shutting down...\n");
        running = 0;
    }
}

void process_controls() 
{
    pthread_mutex_lock(&sensor_shm->mutex);
    SensorData local_sensors[NUM_ROOMS][NUM_SENSORS];
    memcpy(local_sensors, sensor_shm->sensors, sizeof(local_sensors));
    pthread_mutex_unlock(&sensor_shm->mutex);
    
    pthread_mutex_lock(&control_shm->mutex);
    
    for (int room = 0; room < NUM_ROOMS; room++) 
    {
        for (int i = 0; i < NUM_SENSORS; i++) 
        {
            control_shm->sensors[room][i] = local_sensors[room][i];
            ControlData *ctrl = &control_shm->controls[room][i];
            
            ctrl->device_id = i;
            ctrl->room_id = room;
            ctrl->timestamp = time(NULL);
            
            switch(i) 
            {
                case TEMP_SENSOR:
                    strcpy(ctrl->control_name, "Air Conditioner");
                    if (local_sensors[room][i].value > 35.0) 
                    {
                        strcpy(ctrl->action, "Air Conditioner Cooling at 100%");
                        strcpy(ctrl->status, "HIGH TEMP ALERT!");
                        ctrl->alarm_status = 1;
                    } 
                    else if (local_sensors[room][i].value > 28.0) {
                        strcpy(ctrl->action, "Air Conditioner Cooling at 60%");
                        strcpy(ctrl->status, "Active");
                        ctrl->alarm_status = 0;
                    } 
                    else {
                        strcpy(ctrl->action, "Standby");
                        strcpy(ctrl->status, "Air Conditioner OFF - Normal");
                        ctrl->alarm_status = 0;
                    }
                    break;
                    
                case SMOKE_SENSOR:
                    strcpy(ctrl->control_name, "Fire Alarm");
                    if (local_sensors[room][i].value > 50.0) 
                    {
                        strcpy(ctrl->action, "EVACUATE!");
                        strcpy(ctrl->status, "FIRE DETECTED!");
                        ctrl->alarm_status = 1;
                    } 
                    else if (local_sensors[room][i].value > 30.0) 
                    {
                        strcpy(ctrl->action, "Warning");
                        strcpy(ctrl->status, "Smoke Detected");
                        ctrl->alarm_status = 0;
                    } 
                    else {
                        strcpy(ctrl->action, "Monitoring");
                        strcpy(ctrl->status, "Clear");
                        ctrl->alarm_status = 0;
                    }
                    break;
                 case MOTION_SENSOR:
		    strcpy(ctrl->control_name, "Security System");
		    int motion_value = (int)local_sensors[room][i].value;
		    
		    if (motion_value == 0) {
			strcpy(ctrl->action, "Standby");
			strcpy(ctrl->status, "No Activity");
			ctrl->alarm_status = 0;
		    } else if (motion_value == 1) {
			strcpy(ctrl->action, "Recording");
			strcpy(ctrl->status, "Person Detected");
			ctrl->alarm_status = 0;
		    } else if (motion_value == 2) {
			strcpy(ctrl->action, "Alert Mode");
			strcpy(ctrl->status, "Intrusion Alert");
			ctrl->alarm_status = 1;
		    } else if (motion_value == 3) {
			strcpy(ctrl->action, "Tracking");
			strcpy(ctrl->status, "Pet Movement");
			ctrl->alarm_status = 0;
		    } else {
			strcpy(ctrl->action, "Monitoring");
			strcpy(ctrl->status, "Unknown Motion");
			ctrl->alarm_status = 0;
		    }
		    break;
		 case DOOR_LOCK:
                    strcpy(ctrl->control_name, "Smart Lock");
                    if (strncmp(local_sensors[room][i].door_lock_id, "VALID",5)==0) {
                        strcpy(ctrl->action, "Unlocked");
                        strcpy(ctrl->status, "Access Granted");
                        ctrl->alarm_status = 0;
                    } 
                    else {
                        strcpy(ctrl->action, "Locked");
                        strcpy(ctrl->status, "Invalid Access");
                        ctrl->alarm_status = 1;
                    }
                    break;
                    
                case HUMIDITY_SENSOR:
                    strcpy(ctrl->control_name, "Dehumidifier");
                    if (local_sensors[room][i].value > 70.0) {
                        strcpy(ctrl->action, "Dehumidifier High Speed");
                        strcpy(ctrl->status, "High Humidity");
                        ctrl->alarm_status = 0;
                    } 
                    else if (local_sensors[room][i].value > 60.0) {
                        strcpy(ctrl->action, "Dehumidifier Low Speed");
                        strcpy(ctrl->status, "Active");
                        ctrl->alarm_status = 0;
                    } 
                    else {
                        strcpy(ctrl->action, "Dehumidifier Off");
                        strcpy(ctrl->status, "Normal");
                        ctrl->alarm_status = 0;
                    }
                    break;
                    
                case LIGHT_SENSOR:
                    strcpy(ctrl->control_name, "Smart Blinds");
                    if (local_sensors[room][i].value > 400) {
                        strcpy(ctrl->action, "LIGHT OFF");//Closing
                        strcpy(ctrl->status, "Too Bright");
                        ctrl->alarm_status = 0;
                    } 
                    else  { //(local_sensors[room][i].value <=400)
                        strcpy(ctrl->action, "LIGHT ON");//Opening
                        strcpy(ctrl->status, "Too Dark");
                        ctrl->alarm_status = 0;
                    } /*else {
                        strcpy(ctrl->action, "Hold");
                        strcpy(ctrl->status, "Optimal");
                        ctrl->alarm_status = 0;
                    }*/
                    break;
            }
        }
    }
    
    control_shm->batch_count++;
    pthread_mutex_unlock(&control_shm->mutex);
}

int main() {
    signal(SIGINT, signal_handler);
    
    printf("[DPCP] Control Processor starting...\n");
    
    int shm_sensor = shmget(SHM_SENSOR_KEY, sizeof(SensorSharedMemory), 0666);
    sensor_shm = (SensorSharedMemory*)shmat(shm_sensor, NULL, 0);
    
    int shm_control = shmget(SHM_CONTROL_KEY, sizeof(ControlSharedMemory), IPC_CREAT | 0666);
    control_shm = (ControlSharedMemory*)shmat(shm_control, NULL, 0);
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&control_shm->mutex, &attr);
    control_shm->batch_count = 0;
    
    printf("[DPCP] Processing %d rooms\n", NUM_ROOMS);
    
    while(running) {
        process_controls();
        sleep(1);
    }
    
    shmdt(sensor_shm);
    shmdt(control_shm);
    return 0;
}

