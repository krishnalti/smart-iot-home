#include "common.h"
#include <ncurses.h>

ControlSharedMemory *control_shm;
volatile sig_atomic_t running = 1;
int current_room = 0;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        running = 0;
    }
}

void init_ncurses() {
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_BLUE);
    init_pair(6, COLOR_BLACK, COLOR_WHITE);
}

void display_dashboard() {
    clear();
    
    pthread_mutex_lock(&control_shm->mutex);
    SensorData sensors[NUM_ROOMS][NUM_SENSORS];
    ControlData controls[NUM_ROOMS][NUM_SENSORS];
    int batch = control_shm->batch_count;
    memcpy(sensors, control_shm->sensors, sizeof(sensors));
    memcpy(controls, control_shm->controls, sizeof(controls));
    pthread_mutex_unlock(&control_shm->mutex);
    
    int row = 0;
    
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(row++, 0, " IoT SMART HOME - MULTI-ROOM DASHBOARD ");
    attroff(COLOR_PAIR(5) | A_BOLD);
    
    row++;
    mvprintw(row++, 0, "Batch: %d | Press 1 for Room 1 | Press 2 for Room 2 | Press Q to quit", batch);
    row++;
    
    // Display room tabs
    for (int r = 0; r < NUM_ROOMS; r++) {
        if (r == current_room) {
            attron(COLOR_PAIR(6) | A_BOLD);
            mvprintw(row, r * 15, " ROOM %d ", r + 1);
            attroff(COLOR_PAIR(6) | A_BOLD);
        } else {
            attron(COLOR_PAIR(4));
            mvprintw(row, r * 15, " Room %d ", r + 1);
            attroff(COLOR_PAIR(4));
        }
    }
    row += 2;
    
    mvprintw(row++, 0, "=================================================================");
    mvprintw(row++, 0, "%-20s %-10s %-25s %-15s", "SENSOR", "VALUE", "CONTROL ACTION", "STATUS");
    mvprintw(row++, 0, "=================================================================");
    
    const char *sensor_names[] = {"Temperature", "Smoke", "Motion", "Door Lock", "Humidity", "Light"};
    
    for (int i = 0; i < NUM_SENSORS; i++) {
        char value_str[32];
        if (i == 0) sprintf(value_str, "%.1f°C", sensors[current_room][i].value);
        else if (i == 1) sprintf(value_str, "%.0f ppm", sensors[current_room][i].value);
        else if (i == 2) sprintf(value_str, "%s", sensors[current_room][i].value > 0 ? "YES" : "NO");
        else if (i == 4) sprintf(value_str, "%.0f%%", sensors[current_room][i].value);
        else if (i == 5) sprintf(value_str, "%.0f lux", sensors[current_room][i].value);
        else sprintf(value_str, "%s", sensors[current_room][i].door_lock_id);
        
        int color = controls[current_room][i].alarm_status ? 2 : 1;
        attron(COLOR_PAIR(color));
        mvprintw(row++, 0, "%-20s %-10s %-25s %-15s",
                 sensor_names[i], value_str,
                 controls[current_room][i].action,
                 controls[current_room][i].status);
        attroff(COLOR_PAIR(color));
    }
    
    refresh();
}

int main() {
    signal(SIGINT, signal_handler);
    
    int shm_id = shmget(SHM_CONTROL_KEY, sizeof(ControlSharedMemory), 0666);
    control_shm = (ControlSharedMemory*)shmat(shm_id, NULL, 0);
    
    init_ncurses();
    
    while(running) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
        if (ch == '1') current_room = 0;
        if (ch == '2' && NUM_ROOMS > 1) current_room = 1;
        
        display_dashboard();
        usleep(500000);
    }
    
    endwin();
    shmdt(control_shm);
    return 0;
}

