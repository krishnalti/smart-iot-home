#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

ControlSharedMemory *control_shm;
volatile sig_atomic_t running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\n[WEB UI] Shutting down...\n");
        running = 0;
    }
}

void generate_html_dashboard(char* buffer, size_t buffer_size, int room_id) {
    pthread_mutex_lock(&control_shm->mutex);
    SensorData sensors[NUM_SENSORS];
    ControlData controls[NUM_SENSORS];
    int batch = control_shm->batch_count;
    
    memcpy(sensors, control_shm->sensors[room_id], sizeof(sensors));
    memcpy(controls, control_shm->controls[room_id], sizeof(controls));
    pthread_mutex_unlock(&control_shm->mutex);
    
    snprintf(buffer, buffer_size,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <meta charset='UTF-8'>\n"
        "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
        "  <meta http-equiv='refresh' content='2'>\n"
        "  <title>🏠 IoT Smart Home - Room %d</title>\n"
        "  <style>\n"
        "    * { margin: 0; padding: 0; box-sizing: border-box; }\n"
        "    body { font-family: 'Segoe UI', Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%%, #764ba2 100%%); padding: 20px; }\n"
        "    .container { max-width: 1200px; margin: 0 auto; }\n"
        "    h1 { text-align: center; color: white; margin-bottom: 10px; font-size: 2.5em; }\n"
        "    .room-tabs { display: flex; justify-content: center; gap: 10px; margin-bottom: 20px; }\n"
        "    .room-tab { padding: 12px 30px; background: rgba(255,255,255,0.2); color: white; text-decoration: none; border-radius: 8px; font-weight: bold; transition: 0.3s; }\n"
        "    .room-tab:hover { background: rgba(255,255,255,0.3); }\n"
        "    .room-tab.active { background: white; color: #667eea; }\n"
        "    .info { text-align: center; color: white; margin-bottom: 20px; }\n"
        "    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }\n"
        "    .card { background: white; border-radius: 15px; padding: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.2); transition: transform 0.3s; }\n"
        "    .card:hover { transform: translateY(-5px); }\n"
        "    .card.alarm { border: 3px solid #e74c3c; animation: pulse 1s infinite; }\n"
        "    @keyframes pulse { 0%%, 100%% { box-shadow: 0 0 0 0 rgba(231, 76, 60, 0.7); } 50%% { box-shadow: 0 0 0 10px rgba(231, 76, 60, 0); } }\n"
        "    .sensor-header { display: flex; align-items: center; margin-bottom: 15px; }\n"
        "    .sensor-icon { font-size: 2em; margin-right: 10px; }\n"
        "    .sensor-name { font-size: 1.2em; font-weight: bold; color: #2c3e50; }\n"
        "    .sensor-value { font-size: 2.5em; font-weight: bold; color: #3498db; margin: 10px 0; }\n"
        "    .control-info { background: #ecf0f1; padding: 10px; border-radius: 8px; margin-top: 10px; }\n"
        "    .control-action { font-weight: bold; color: #27ae60; }\n"
        "    .control-status { color: #7f8c8d; margin-top: 5px; }\n"
        "    .status-alarm { color: #e74c3c !important; font-weight: bold; }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <div class='container'>\n"
        "    <h1>🏠 IoT Smart Home Dashboard</h1>\n"
        "    <div class='room-tabs'>\n"
        "      <a href='/?room=0' class='room-tab %s'>Room 1</a>\n"
        "      <a href='/?room=1' class='room-tab %s'>Room 2</a>\n"
        "    </div>\n"
        "    <div class='info'>🔄 LIVE - Batch: %d | Room %d</div>\n"
        "    <div class='grid'>\n",
        room_id + 1,
        room_id == 0 ? "active" : "",
        room_id == 1 ? "active" : "",
        batch, room_id + 1);
    
    const char* icons[] = {"🌡️", "💨", "👁️", "🚪", "💧", "💡"};
    const char* names[] = {"Temperature", "Smoke Detector", "Motion Sensor", "Door Lock", "Humidity", "Light Sensor"};
    
    for (int i = 0; i < NUM_SENSORS; i++) {
        char card[2048];
        char value_str[64];
        
        if (i == 0) snprintf(value_str, sizeof(value_str), "%.1f°C", sensors[i].value);
        else if (i == 1) snprintf(value_str, sizeof(value_str), "%.0f ppm", sensors[i].value);
        else if (i == 2) snprintf(value_str, sizeof(value_str), "%s", sensors[i].value > 0 ? "DETECTED" : "CLEAR");
        else if (i == 3) snprintf(value_str, sizeof(value_str), "%s", sensors[i].door_lock_id);
        else if (i == 4) snprintf(value_str, sizeof(value_str), "%.0f%%", sensors[i].value);
        else if (i == 5) snprintf(value_str, sizeof(value_str), "%.0f lux", sensors[i].value);
        
        snprintf(card, sizeof(card),
            "      <div class='card%s'>\n"
            "        <div class='sensor-header'>\n"
            "          <div class='sensor-icon'>%s</div>\n"
            "          <div class='sensor-name'>%s</div>\n"
            "        </div>\n"
            "        <div class='sensor-value'>%s</div>\n"
            "        <div class='control-info'>\n"
            "          <div class='control-action'>⚡ %s</div>\n"
            "          <div class='control-status%s'>%s</div>\n"
            "        </div>\n"
            "      </div>\n",
            controls[i].alarm_status ? " alarm" : "",
            icons[i], names[i], value_str,
            controls[i].action,
            controls[i].alarm_status ? " status-alarm" : "",
            controls[i].status);
        
        strcat(buffer, card);
    }
    
    strcat(buffer, "    </div>\n  </div>\n</body>\n</html>");
}

int main() {
    signal(SIGINT, signal_handler);
    
    printf("[WEB UI] Starting web server...\n");
    
    int shm_id = shmget(SHM_CONTROL_KEY, sizeof(ControlSharedMemory), 0666);
    control_shm = (ControlSharedMemory*)shmat(shm_id, NULL, 0);
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(WEB_SERVER_PORT);
    
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);
    
    printf("[WEB UI] Listening on port %d\n", WEB_SERVER_PORT);
    printf("[WEB UI] Access: http://<server-ip>:%d\n", WEB_SERVER_PORT);
    
    while(running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) continue;
        
        char request[4096];
        read(client_fd, request, sizeof(request));
        
        int room_id = 0;
        if (strstr(request, "room=1")) room_id = 1;
        
        char response[16384];
        generate_html_dashboard(response, sizeof(response), room_id);
        write(client_fd, response, strlen(response));
        close(client_fd);
    }
    
    close(server_fd);
    shmdt(control_shm);
    return 0;
}

