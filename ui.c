#include <stdio.h>

// ANSI color macros
#define RESET           "\x1b[0m"
#define BOLD            "\x1b[1m"
#define WHITE           "\x1b[37m"
#define BG_MAGENTA      "\x1b[45m"
#define BRIGHT_MAGENTA  "\x1b[95m"
#define BRIGHT_WHITE    "\x1b[97m"
#define BRIGHT_CYAN     "\x1b[96m"

// Device structure (example)
typedef struct {
    int id;
    char sensor_type[32];
    char value[16];
    char actuator_type[32];
    char power[16];
    char status[64];
} Device;

// Sample data
Device devices[] = {
    {1, "Temperature Sensor", "23.5°C", "Fan", "ON", "Normal"},
    {2, "Humidity Sensor", "45%", "Dehumidifier", "OFF", "Idle"},
    {3, "Motion Sensor", "Active", "Alarm", "ON", "Triggered"},
};

int num_devices = sizeof(devices) / sizeof(devices[0]);

// Helper function to print a dashboard row
void print_dashboard_row(int id, const char* sensor_type, const char* value,
                         const char* actuator_type, const char* power, const char* status) {
    printf("%s│ %2d │ %-20s │ %-12s │ %-18s │ %-10s │ %-40s │%s\n",
           BRIGHT_MAGENTA, id, sensor_type, value, actuator_type, power, status, RESET);
}

// Optional separator line
void print_separator() {
    printf("%s%s%s┼────┼──────────────────────┼────────────┼────────────────────┼────────────┼──────────────────────────────────────────┼%s\n",
           BOLD, BG_MAGENTA, WHITE, RESET);
}

// Main dashboard display function
void display_dashboard() {
    // Header
    printf("%s%s%s│ ID │ SENSOR TYPE           │ VALUE        │ ACTUATOR TYPE      │ POWER      │ STATUS                                   │%s\n",
           BOLD, BG_MAGENTA, WHITE, RESET);
    print_separator();

    // Rows
    for (int i = 0; i < num_devices; i++) {
        print_dashboard_row(devices[i].id,
                            devices[i].sensor_type,
                            devices[i].value,
                            devices[i].actuator_type,
                            devices[i].power,
                            devices[i].status);
    }

    // Footer spacing
    printf("\n");
}

// Entry point
int main() {
    display_dashboard();
    return 0;
}

