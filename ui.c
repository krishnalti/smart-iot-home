#include "common.h"

SharedMemory *shm_ptr;
sem_t *sem_dpcp, *sem_ui;

// ANSI Color Codes
#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define DIM         "\033[2m"

#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"

#define BRIGHT_BLACK   "\033[90m"
#define BRIGHT_RED     "\033[91m"
#define BRIGHT_GREEN   "\033[92m"
#define BRIGHT_YELLOW  "\033[93m"
#define BRIGHT_BLUE    "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN    "\033[96m"
#define BRIGHT_WHITE   "\033[97m"

#define BG_BLACK    "\033[40m"
#define BG_RED      "\033[41m"
#define BG_GREEN    "\033[42m"
#define BG_YELLOW   "\033[43m"
#define BG_BLUE     "\033[44m"
#define BG_CYAN     "\033[46m"
#define BG_WHITE    "\033[47m"

#define BG_BRIGHT_BLACK   "\033[100m"
#define BG_BRIGHT_RED     "\033[101m"
#define BG_BRIGHT_GREEN   "\033[102m"
#define BG_BRIGHT_YELLOW  "\033[103m"
#define BG_BRIGHT_BLUE    "\033[104m"
#define BG_BRIGHT_MAGENTA "\033[105m"
#define BG_BRIGHT_CYAN    "\033[106m"
#define BG_BRIGHT_WHITE   "\033[107m"

void clear_screen() {
    printf("\033[2J\033[H");
}

void display_dashboard() {
    clear_screen();

    // Title
    printf(BOLD BRIGHT_CYAN BG_BLUE "╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD BRIGHT_WHITE BG_BLUE "║                                         IoT SMART HOME DASHBOARD                                                         ║\n" RESET);
    printf(BOLD BRIGHT_CYAN BG_BLUE "╚════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n" RESET);
    printf(BRIGHT_YELLOW "                                         Last Updated: %s" RESET, ctime(&(time_t){time(NULL)}));
    printf("\n");

    // Table header
    printf(BRIGHT_MAGENTA "┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n" RESET);
    printf(BOLD BG_MAGENTA WHITE "│ ID │ SENSOR TYPE           │ VALUE        │ ACTUATOR TYPE      │ POWER      │ STATUS                                   │\n" RESET);
    printf(BRIGHT_MAGENTA "├────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤\n" RESET);

    for (int i = 0; i < NUM_SENSORS; i++) {
        SensorData sensor = shm_ptr->sensors[i];
        ActuatorData actuator = shm_ptr->actuators[i];

        char sensor_value[20];
        const char* value_color = BRIGHT_GREEN;

        switch(sensor.type) {
            case TEMP_SENSOR:
                snprintf(sensor_value, sizeof(sensor_value), "%.1f°C", sensor.value);
                if (sensor.value > 30) value_color = BRIGHT_RED;
                else if (sensor.value > 25) value_color = BRIGHT_YELLOW;
                break;
            case LIGHT_SENSOR:
                snprintf(sensor_value, sizeof(sensor_value), "%.0f lux", sensor.value);
                value_color = BRIGHT_YELLOW;
                break;
            case GAS_SENSOR:
                snprintf(sensor_value, sizeof(sensor_value), "%.0f ppm", sensor.value);
                if (sensor.value > 500) value_color = BRIGHT_RED;
                else if (sensor.value > 300) value_color = BRIGHT_YELLOW;
                break;
            case MOTION_SENSOR:
                if (sensor.value > 0.0) {
                    snprintf(sensor_value, sizeof(sensor_value), "Detected");
                    value_color = BRIGHT_GREEN;
                } else {
                    snprintf(sensor_value, sizeof(sensor_value), "Not Detect");
                    value_color = WHITE;
                }
                break;
        }

        char action_value[20];
        const char* action_color = BRIGHT_CYAN;
        if (actuator.type == SECURITY_ALARM) {
            snprintf(action_value, sizeof(action_value), "%s", actuator.value >= 1.0 ? "TRIGGERED" : "OFF");
            action_color = actuator.value >= 1.0 ? BRIGHT_RED : BRIGHT_GREEN;
        } else {
            snprintf(action_value, sizeof(action_value), "%.0f%%", actuator.value);
            if (actuator.value > 75) action_color = BRIGHT_YELLOW;
        }

        const char* alarm_indicator = actuator.alarm_status ? BRIGHT_RED " ⚠️  " RESET : "    ";
        const char* row_bg = (i % 2 == 0) ? BG_BRIGHT_BLACK : "";

        char status_display[40];
        const char* status_color = BRIGHT_WHITE;

        if (strstr(actuator.status, "Alert") || strstr(actuator.status, "TRIGGERED")) {
            status_color = BRIGHT_RED;
        } else if (strstr(actuator.status, "OFF")) {
            status_color = BRIGHT_YELLOW;
        } else {
            status_color = BRIGHT_GREEN;
        }

        if (strlen(actuator.status) > 35) {
            snprintf(status_display, sizeof(status_display), "%.32s...", actuator.status);
        } else {
            snprintf(status_display, sizeof(status_display), "%-35s", actuator.status);
        }

        printf("%s" BRIGHT_MAGENTA "│ " BRIGHT_WHITE "%2d " BRIGHT_MAGENTA "│ " BRIGHT_CYAN "%-20s " BRIGHT_MAGENTA "│ "
               "%s%-12s" RESET " " BRIGHT_MAGENTA " │ " BRIGHT_YELLOW "%-18s " BRIGHT_MAGENTA " │ "
               "%s%-10s" RESET " " BRIGHT_MAGENTA " │ " "%s%s" RESET " " BRIGHT_MAGENTA " │\n" RESET,
               row_bg, i + 1, sensor.name, value_color, sensor_value,
               actuator.name, action_color, action_value, alarm_indicator, status_color, status_display);
    }

    printf(BRIGHT_MAGENTA "└────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘\n" RESET);

    // Alarm summary
    int alarm_count = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (shm_ptr->actuators[i].alarm_status) {
            alarm_count++;
        }
    }

    if (alarm_count > 0) {
        printf("\n" BOLD BRIGHT_RED BG_YELLOW " ⚠️  ACTIVE ALARMS: %d " RESET "\n", alarm_count);
        printf(BRIGHT_RED "┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n" RESET);
        for (int i = 0; i < NUM_SENSORS; i++) {
            if (shm_ptr->actuators[i].alarm_status) {
                printf(BG_RED WHITE "│ ⚠️  %s: %s" RESET "\n", shm_ptr->actuators[i].name, shm_ptr->actuators[i].status);
            }
        }
        printf(BRIGHT_RED "└────────────────────────────────────────────────────────────────────────────────────────────────────────┘\n" RESET);
    } else {
        printf("\n" BOLD BRIGHT_GREEN "✅️ All systems normal - No active alarms" RESET "\n");
    }

    printf("\n" BRIGHT_YELLOW "Press Ctrl+C to exit..." RESET "\n");
}

int main() {
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

    sem_dpcp = sem_open(SEM_DPCP, 0);
    sem_ui = sem_open(SEM_UI, 0);

    if (sem_dpcp == SEM_FAILED || sem_ui == SEM_FAILED) {
        perror("Semaphore open failed");
        exit(1);
    }

    printf(BRIGHT_CYAN "[UI] User Interface Process started\n" RESET);
    printf(BRIGHT_YELLOW "[UI] Waiting for DPCP to process first batch...\n" RESET);

    sem_wait(sem_dpcp);

    printf(BRIGHT_GREEN "[UI] Received signal from DPCP. Starting dashboard...\n" RESET);
    sleep(1);

    while(1) {
        display_dashboard();
        sleep(UPDATE_INTERVAL);
    }

    shmdt(shm_ptr);
    sem_close(sem_dpcp);
    sem_close(sem_ui);

    return 0;
}
