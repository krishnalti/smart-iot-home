Based on the code analysis, this is a distributed IoT smart home monitoring system with client-server architecture, multi-room sensor support, and dual UI interfaces (terminal and web). Here's a comprehensive README for the project:​​

IoT Smart Home Multi-Room Monitoring System
A distributed IoT system that monitors and controls smart home devices across multiple rooms using a client-server architecture with real-time data processing and dual user interfaces.

Overview
This system simulates a smart home environment with IoT devices distributed across two rooms, featuring real-time sensor monitoring and automated control responses. The architecture separates client-side device simulators from server-side processing and visualization components, enabling deployment across different machines.

Key Features
Multi-room support: Monitors 6 sensor types across 2 rooms (12 devices total)

Distributed architecture: Client and server components run on separate machines

Real-time processing: Continuous sensor data collection and control logic execution

Dual UI: Terminal-based (ncurses) and web-based dashboards

Automated controls: Intelligent responses to sensor readings (AC, alarms, locks, etc.)

Thread-safe communication: Uses shared memory with mutex synchronization

CLIENT MACHINE                    SERVER MACHINE
┌─────────────────┐              ┌──────────────────┐
│  dcp_client          │─────TCP───>│  dcp_server       |
│  (IoT Devices)       │   Port 8080   │  (Data Receiver)     |    
└─────────────────┘              └────────┬─────────┘
                                          │ Shared Memory
                                          ▼
                                 ┌────────────────┐
                                 │     dpcp            │
                                 │ (Control Logic)     │
                                 └────────┬───────┘
                                          │ Shared Memory
                          ┌───────────────┴───────────────┐
                          ▼                               ▼
                   ┌─────────────┐              ┌─────────────┐
                   │     ui          │              │   web_ui    │
                   │ (ncurses)       │              │ (HTTP:80)   │
                   └─────────────┘              └─────────────┘
Components
Client Machine:

dcp_client: Simulates IoT sensors, generates data, and sends to server via TCP

Server Machine:

dcp_server: TCP server receiving sensor data from clients (port 8080)

dpcp: Data Processing and Control Processor implementing control logic

ui: Terminal dashboard using ncurses for real-time monitoring

web_ui: HTTP server (port 80) providing web-based dashboard

Monitored Devices
Each room includes:

Temperature Sensor → Controls Air Conditioner

Smoke Detector → Triggers Fire Alarm

Motion Sensor → Activates Security System

Door Lock → Smart Lock with access validation

Humidity Sensor → Controls Dehumidifier

Light Sensor → Adjusts Smart Blinds/Lights

Prerequisites
Software Requirements
Linux/Unix system (tested on Ubuntu)

GCC compiler

pthread library

ncurses library

Network connectivity between client and server machines

Install Dependencies
sudo apt-get update
sudo apt-get install build-essential libncurses5-dev libncursesw5-dev libcap2-bin

Or use the provided Makefile:
make install-deps

Installation & Setup
1. Configure Server IP Address
Edit common.h on both machines and set the server's IP address:
#define SERVER_IP "192.168.1.XXX"  // Replace with actual server IP


2. Build Components
On Client Machine:
make client


On Server Machine:
make server
make setcap  # Allows web_ui to bind to port 80

Usage
Starting the Server
On the server machine:
./launch_server.sh


This launches four terminal windows:

DCP Server: Listening on port 8080

DPCP: Processing control logic

Web UI: HTTP server on port 80

Terminal UI: Interactive ncurses dashboard

Access the web dashboard at: http://<SERVER_IP>

Starting the Client
On the client machine:
./launch_client.sh

The script will:

Verify the server IP configuration

Test connectivity to the server

Launch IoT device simulators

Using the Interfaces
Terminal UI (ncurses):

Press 1 to view Room 1

Press 2 to view Room 2

Press Q to quit

Web UI:

Navigate to http://<SERVER_IP>

Click "Room 1" or "Room 2" tabs

Auto-refresh to see live updates

Configuration
Key parameters in common.h:
#define NUM_ROOMS 2              // Number of rooms
#define NUM_SENSORS 6            // Sensors per room
#define DCP_SERVER_PORT 8080     // TCP server port
#define WEB_SERVER_PORT 80       // Web UI port
#define UPDATE_INTERVAL 5        // Sensor update interval (seconds)
#define SHM_SENSOR_KEY 1234      // Shared memory key for sensor data
#define SHM_CONTROL_KEY 5678     // Shared memory key for control data

Control Logic
The DPCP component implements automated responses:
Sensor       |  Threshold     |  Action                              
-------------+----------------+--------------------------------------
Temperature  |  > 35°C        |  AC cooling at 100% + HIGH TEMP ALERT
Temperature  |  > 28°C        |  AC cooling at 60%                   
Smoke        |  > 50 ppm      |  EVACUATE! FIRE DETECTED             
Smoke        |  > 30 ppm      |  Warning: Smoke Detected             
Motion       |  Value 2       |  Intrusion Alert                     
Door Lock    |  Invalid code  |  Locked - Invalid Access             
Humidity     |  > 70%         |  Dehumidifier High Speed             
Light        |  > 400 lux     |  Turn lights off                     
Light        |  ≤ 400 lux     |  Turn lights on                      


├── common.h              # Shared definitions and data structures
├── dcp_client.c          # Client: IoT device simulator
├── dcp_server.c          # Server: TCP data receiver
├── dpcp.c                # Server: Control processing logic
├── ui.c                  # Server: ncurses terminal UI
├── web_ui.c              # Server: HTTP web UI
├── Makefile              # Build automation
├── launch_client.sh      # Client startup script
└── launch_server.sh      # Server startup script


Troubleshooting
Connection Issues
Cannot connect to server:

Verify SERVER_IP in common.h is correct

Check firewall settings: sudo ufw allow 8080

Ensure server is running before starting client

Test connectivity: telnet <SERVER_IP> 8080

Web UI not accessible:

Run make setcap on server machine

Check if port 80 is available: sudo netstat -tulpn | grep :80

Try alternative port by modifying WEB_SERVER_PORT in common.h

Shared Memory Issues
If processes fail to start or show shared memory errors:
# On server machine
make clean-shm

Or manually:
ipcrm -M 1234
ipcrm -M 5678
rm -f /dev/shm/sem.sem_*


Build Errors
# Clean and rebuild
make clean
make client  # On client machine
make server  # On server machine



Development
Adding New Sensor Types
Add sensor enum to SensorType in common.h

Update NUM_SENSORS value

Implement data generation in dcp_client.c

Add control logic in dpcp.c

Update UI displays in ui.c and web_ui.c

Extending to More Rooms

Modify NUM_ROOMS in common.h

Rebuild all components

Update UI navigation logic


Signal Handling
All processes implement proper SIGINT (Ctrl+C) handling for clean shutdown:

Closes network sockets

Detaches shared memory

Releases mutex locks

Terminates gracefully


Performance Considerations
Update interval: Default 5 seconds, configurable via UPDATE_INTERVAL

Concurrent connections: Server supports up to 20 simultaneous client connections

Shared memory: Uses process-shared mutexes for thread-safe access

TCP buffering: 256-byte buffers for sensor data transmission


Security Notes
System uses unencrypted TCP communication (consider TLS for production)

Web UI has no authentication (add access control for production)

Runs with elevated capabilities for port 80 binding

Shared memory accessible to all users with permissions


Acknowledgments
Uses POSIX shared memory for IPC

ncurses library for terminal UI

Multi-threaded design with pthread


