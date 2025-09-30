/*
 * Konfigurasi untuk ESP32 Board 3.3.1
 * File ini berisi konfigurasi khusus untuk ESP32 3.3.1
 */

#ifndef ESP32_3_3_1_CONFIG_H
#define ESP32_3_3_1_CONFIG_H

// Konfigurasi Pin untuk ESP32 3.3.1
#define SW_SERIAL_TX 25
#define SW_SERIAL_RX 26

// Pin untuk MAX3485 (Modbus RTU)
#define PIN_RO 32  // Receive Output
#define PIN_RE 33  // Receive Enable

// Pin untuk Ethernet (W5500)
#define ETH_CS_PIN 5

// Pin untuk Servo (8 port)
#define SERVO_PIN_1 14
#define SERVO_PIN_2 22
#define SERVO_PIN_3 17
#define SERVO_PIN_4 35
#define SERVO_PIN_5 21
#define SERVO_PIN_6 27
#define SERVO_PIN_7 16
#define SERVO_PIN_8 13

// Konfigurasi Task untuk ESP32 3.3.1
#define SENSOR_TASK_STACK_SIZE 8192
#define MODBUS_TASK_STACK_SIZE 8192
#define SENSOR_TASK_PRIORITY 1
#define MODBUS_TASK_PRIORITY 1

// Konfigurasi Memory untuk ESP32 3.3.1
#define JSON_BUFFER_SIZE 1024
#define JSON_DOC_SIZE 384

// Konfigurasi Network untuk ESP32 3.3.1
#define WIFI_AP_SSID "ExtenderAP"
#define WIFI_AP_PASSWORD "12345678"
#define WIFI_STATION_SSID "Synapsis.id Head Office"
#define WIFI_STATION_PASSWORD "synaps1sjkt23@"

// Konfigurasi Ethernet untuk ESP32 3.3.1
#define ETH_MAC_ADDRESS "24:0A:C4:0A:65:AE"
#define ETH_IP_DEFAULT "192.168.1.100"
#define ETH_GATEWAY_DEFAULT "192.168.1.1"
#define ETH_SUBNET_DEFAULT "255.255.255.0"
#define ETH_DNS_DEFAULT "8.8.8.8"

// Konfigurasi Modbus untuk ESP32 3.3.1
#define MODBUS_SLAVE_ID_DEFAULT 9
#define MODBUS_BAUDRATE 9600
#define MODBUS_TIMEOUT 1000

// Konfigurasi Servo untuk ESP32 3.3.1
#define SERVO_STOP_PULSE 90
#define SERVO_PUSH_SPEED 30
#define SERVO_PUSH_MS 600
#define SERVO_SETTLE_MS 150
#define SERVO_BRAKE_SPEED 120
#define SERVO_BRAKE_MS 120
#define SERVO_DECEL_MS 150
#define SERVO_DECEL_STEPS 3

// Konfigurasi File System untuk ESP32 3.3.1
#define FS_MOUNT_PATH "/"
#define FS_MAX_OPEN_FILES 5

// Konfigurasi Web Server untuk ESP32 3.3.1
#define WIFI_WEB_SERVER_PORT 8000  // WiFi WebServer port
#define ETHERNET_WEB_SERVER_PORT 80  // Ethernet WebServer port
#define WEB_SERVER_MAX_URI_LENGTH 50

// Konfigurasi Sensor untuk ESP32 3.3.1
#define NUM_PORTS 8
#define SENSOR_READ_INTERVAL 1000
#define SENSOR_ERROR_TOLERANCE 10

// Konfigurasi Modbus Registers untuk ESP32 3.3.1
#define REG_TYPE_START 100
#define REG_VALUE_START 200
#define REG_STATE_START 300
#define REG_PORT_MODBUS_ID_START 400
#define REG_PORT_PIN_START 500

// Konfigurasi Debug untuk ESP32 3.3.1
#define DEBUG_SERIAL_BAUD 115200
#define DEBUG_ENABLED true

// Konfigurasi OTA untuk ESP32 3.3.1
#define OTA_HOSTNAME "ExtenderEnvDevice"
#define OTA_PASSWORD "12345678"

// Konfigurasi Memory Management untuk ESP32 3.3.1
#define HEAP_SIZE_MONITORING true
#define MIN_FREE_HEAP_SIZE 10000

// Konfigurasi Watchdog untuk ESP32 3.3.1
#define WATCHDOG_TIMEOUT 30
#define WATCHDOG_ENABLED true

#endif // ESP32_3_3_1_CONFIG_H

