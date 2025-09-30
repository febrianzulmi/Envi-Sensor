// Version 1.0.1 (23 Januari 2025)

#include "config.h"  // ESP32 3.3.1 Configuration
#include <ArduinoJson.h>
#include <NearonLibrary.h>  // Updated for ESP32 3.3.1 compatibility
#include <WebServer.h>
#include <Ethernet.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include <ESP32Servo.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <exception>


Memory filemanager;
const String networkAPpath = "/network/wifi_ap.json";
const String networkSTApath = "/network/wifi_sta.json";
const String configPortpath = "/config/portconfig.json";
const String configDevicepath = "/config/deviceconfig.json";
const String addressConfigEth = "/network/eth.json";


Connection Connections;
const String ssid_ap = "ExtenderAP";
const String password_ap = "12345678";
String ssid_station = "Synapsis.id Head Office";
String password_station = "synaps1sjkt23@";
// String ssid_station = "solarmast";
// String password_station = "12345678";
String ip_station, ip_ap, mac_ap, mac_station, wifi_rssi;
int wifi_status;
int dual_wifi = 0;
int couter_reset_network_if_not_connected_10_times;
bool trigger_wireless = false;
bool dhcp_wireless;
IPAddress ip_station_conf(0, 0, 0, 0);
IPAddress gateway_station_conf(0, 0, 0, 0);
IPAddress subnet_station_conf(0, 0, 0, 0);
IPAddress dns1_station_conf(8, 8, 8, 8);
IPAddress dns2_station_conf(8, 8, 4, 4);


EthernetClass eth;
const String macETH = "24:0A:C4:0A:65:AE";
uint8_t csPinorSSPin = 5;
IPAddress ip_eth_conf(192, 168, 1, 100);  
IPAddress gateway_eth_conf(192, 168, 1, 1);  // Set default gateway
IPAddress subnet_eth_conf(255, 255, 255, 0);
IPAddress dns1_eth_conf(8, 8, 8, 8);
IPAddress dns2_eth_conf(8, 8, 4, 4);
bool dhcp_eth = true;  // Enable DHCP by default for easier setup
bool trigger_to_change_eth_setup_ontrue;


// Dual WebServer dengan port berbeda
EthernetServer ethernetServer(ETHERNET_WEB_SERVER_PORT);    // EthernetServer untuk Ethernet (port 80)
WebServer wifiServer(WIFI_WEB_SERVER_PORT);      // WebServer untuk WiFi (port 8000)
bool wifiServerActive = false;
bool ethernetServerActive = false;
const String APIrootAddr = "/";
const String APIgetNetworkInfo = "/api/get/network/info";
const String APIsetNetwork = "/api/set/network/wireless";
const String APIsetEthernet = "/api/set/network/ethernet";
const String APIgetSensorData = "/api/get/sensor/data";
const String APIgetPort = "/api/get/port/config";
const String APIsetPort = "/api/set/port/config";
const String APIgetDevice = "/api/get/device/config";
const String APIsetDevice = "/api/set/device/config";
const String APIservoTrigger = "/api/control/servo/trigger";
OTA ota;

//constructor
void setup();
void loop();
void handleRoot();
void getSensorData(WebServer* server);
void setPort(WebServer* server);
void getPort(WebServer* server);
void setDeviceConfig(WebServer* server);
void getDeviceConfig(WebServer* server);
void setNetwork();
void getNetworkInfo(WebServer* server);
void handleNotFound(WebServer* server);
void servoTrigger(WebServer* server);
void setNetworkStation(WebServer* server);
void setNetworkEthernet(WebServer* server);
void handleNetworkStatus(WebServer* server);
void getroot(WebServer* server);
void setupWebServerRoutes(WebServer* server);
void handleEthernetServer();
void handleEthernetAPI(EthernetClient &client, String method, String path, String body);
String handleEthernetPostRequest(String path, String body);
String readLine(EthernetClient &client);
void updateServoMovement();
void executeSpeedTest(int portIndex, unsigned long currentTime);
void WiFiBegin();
void WiFiState();
void WiFiHandle();
void modbus_slave();
bool isNumberString(const String &str);
bool validateModbus(const String &modbus);
bool validatePin(const String &pin);
bool validateStatus(const String &status);
void validatePorts(StaticJsonDocument<384> &req, StaticJsonDocument<1024> &res, String &message, bool &invalid_status);
void convertToResponseFormat(StaticJsonDocument<384> &req, StaticJsonDocument<1024> &res, String &message);
void handlePostError(const String message, const String dataLabel, const String dataInput);
void readConfig();
void readPortConfig();
void readNetworkSTAConfig();
void readNetworkETHConfig();
bool writeNetworkSTAConfig(String ssid, String pass);
bool writeNetworkETHConfig();
void readSensorMain();
void readSensorTask(void *pvParameters);
bool setupSHT20Sensor(int port, uint8_t modbus_id);

class portValue {
public:
  String PORT_TEMP[8];
  String PORT_HUMID[8];
  String smoke_val[8];
  String door_val[8];
  String waterleak_val[8];
  String servo_angle[8];
  String State_port[8];
  String messages[8];

  portValue() {
    for (int i = 0; i < 8; i++) {
      PORT_TEMP[i] = "0";
      PORT_HUMID[i] = "0";
      smoke_val[i] = "0";
      door_val[i] = "0";
      waterleak_val[i] = "0";
      servo_angle[i] = "90";  // Default speed 50ms per degree
      State_port[i] = "disable";
      messages[i] = "no msg";
    }
  }
  bool setMsg(String msg, int port);

  String spewMsg(int port) {
    if (port < 1 || port > 8) {
      return "nomsg";
    } else {
      return messages[port - 1];
    }
  }
};


bool portValue::setMsg(String msg, int port) {
  if (port < 1 || port > 8) {
    return false;
  } else {
    messages[port - 1] = msg;
    return true;
  }
}


class portConfig {
public:

  String PORT_SENSOR[8];
  int PORT_MODBUSID[8];
  int PORT_PIN[8];

  portConfig(String ps[8], int mod_id[8], int port_pin[8]) {
    for (int i = 0; i < 8; i++) {
      PORT_SENSOR[i] = ps[i];
      PORT_MODBUSID[i] = mod_id[i];
      PORT_PIN[i] = port_pin[i];
    }
  }

  void setConfig(String ps[8], int mod_id[8], int port_pin[8]) {
    for (int i = 0; i < 8; i++) {
      PORT_SENSOR[i] = ps[i];
      PORT_MODBUSID[i] = mod_id[i];
      PORT_PIN[i] = port_pin[i];
    }
  }
};

class deviceConfig {
public:

  int MODBUS_SLAVE_ID;
  String MODBUS_SLAVE_STATE, DEVICE_HOSTNAME, DEVICE_NAME, DEVICE_SN;

  deviceConfig() {
    MODBUS_SLAVE_ID = 9;
    MODBUS_SLAVE_STATE = "disable";
    DEVICE_HOSTNAME = "ExtenderEnvDevice";
    DEVICE_NAME = "ExtenderEnvDevice";
    DEVICE_SN = "123456789";
  }

  void setConfig(int id, String state, String hostname, String name, String sn) {
    MODBUS_SLAVE_ID = id;
    MODBUS_SLAVE_STATE = state;
    DEVICE_HOSTNAME = hostname;
    DEVICE_NAME = name;
    DEVICE_SN = sn;
  }
};


// SW_SERIAL_TX dan SW_SERIAL_RX sudah didefinisikan di config.h
SoftwareSerial modbusSerial(SW_SERIAL_RX, SW_SERIAL_TX);  // RX, TX
ModbusMaster sht20[8];
Servo servo[8];
int countErrorSensorPort[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
bool trigger_pause_other_core = true;
bool triggerTochangeSettingPort = true;
portValue VALUE;

// --- Servo continuous rotation configuration ---
// For continuous rotation servos: 90 = stop, <90 spins one way, >90 spins the other
// Semua konfigurasi servo sudah didefinisikan di config.h

// Servo trigger variables
bool servo_trigger_active[8] = { false, false, false, false, false, false, false, false };
unsigned long servo_trigger_start[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int servo_current_angle[8] = { 90, 90, 90, 90, 90, 90, 90, 90 };
deviceConfig DEVICE_CONFIGURATED;
String ps[8] = { "disable", "disable", "disable", "disable", "disable", "disable", "disable", "disable" };
int mod_id[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
int port_pin[8] = { 14, 22, 17, 35, 21, 27, 16, 13 };
portConfig PORT_CONFIGURATED(ps, mod_id, port_pin);
TaskHandle_t SensorTask;
void setup() {

  Serial.begin(115200);
  modbusSerial.begin(9600);

  // System initialization
  Serial.println("ESP32 Environment Extender Device Starting...");

  Serial.println("Initiate File System\n");

  // Try to mount LittleFS with format if failed
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed, trying to format...");
    if (LittleFS.format()) {
      Serial.println("LittleFS formatted successfully!");
      if (LittleFS.begin(true)) {
        Serial.println("LittleFS mounted successfully after format!");
      } else {
        Serial.println("LittleFS still failed to mount after format!");
        while (1) {
          Serial.println("CRITICAL: LittleFS mount failed. Please check hardware.");
          delay(5000);
        }
      }
    } else {
      Serial.println("LittleFS format failed!");
      while (1) {
        Serial.println("CRITICAL: LittleFS format failed. Please check hardware.");
        delay(5000);
      }
    }
  } else {
    Serial.println("LittleFS mounted successfully!");
  }

  filemanager.begin();
  delay(1000);  // Give time for LittleFS to initialize

  // Test file system
  Serial.println("Testing file system...");
  if (filemanager.checkFile("/test.txt")) {
    Serial.println("File system test: PASSED");
  } else {
    Serial.println("File system test: FAILED - trying to create test file");
    filemanager.writeFile("/test.txt", "test");
    if (filemanager.checkFile("/test.txt")) {
      Serial.println("File system test: PASSED after retry");
      filemanager.removeFile("/test.txt");
    } else {
      Serial.println("File system test: CRITICAL FAILURE");
    }
  }

  Serial.println("File System initialized successfully");
  // Ensure required directories exist
  if (!LittleFS.exists("/config")) {
    LittleFS.mkdir("/config");
  }
  if (!LittleFS.exists("/network")) {
    LittleFS.mkdir("/network");
  }

  readConfig();

  // Configuration read completed
  Serial.println("System configuration loaded successfully!");

  WiFiBegin();
  ethBegin();

  Serial.println("Initialization Dynamic Webserver");
  
  // Setup network priority
  setupNetworkPriority();

  Serial.println("Initialization Hostname\n");
  ota.setMDNS(DEVICE_CONFIGURATED.DEVICE_HOSTNAME.c_str());
  ota.setDebug(DEVICE_CONFIGURATED.DEVICE_HOSTNAME.c_str());

  // initializeModbusSlave();

  // Create task for sensor reading - Updated for ESP32 3.3.1
  xTaskCreatePinnedToCore(
    readSensorTask,  // Function to be called
    "SensorTask",    // Name of the task
    8192,            // Stack size (bytes) - reduced for ESP32 3.3.1
    NULL,            // Parameter to pass
    1,               // Task priority
    &SensorTask,     // Task handle
    0                // Core where the task should run
  );

  xTaskCreatePinnedToCore(
    modbusSlaveTask,
    "ModbusSlaveTask",
    8192,            // Stack size (bytes) - reduced for ESP32 3.3.1
    NULL,
    1,
    NULL,
    1);
}

void setupNetworkPriority() {
  Serial.println("=== Dual WebServer Setup ===");
  
  // Setup WiFi WebServer (Port 8000) - Menggunakan WebServer library
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WiFi WebServer on port " + String(WIFI_WEB_SERVER_PORT));
    Serial.println("🌐 WiFi WebServer available at: " + WiFi.localIP().toString() + ":" + String(WIFI_WEB_SERVER_PORT));
    Serial.println("📡 WiFi API access: " + WiFi.localIP().toString() + ":" + String(WIFI_WEB_SERVER_PORT));
    Serial.println("🔗 WiFi Test URL: http://" + WiFi.localIP().toString() + ":" + String(WIFI_WEB_SERVER_PORT) + "/api/get/network/info");
    
    if (!wifiServerActive) {
      Serial.println("🚀 Starting WiFi WebServer...");
      setupWebServerRoutes(&wifiServer);
      wifiServer.begin();
      wifiServerActive = true;
    }
  } else {
    Serial.println("❌ WiFi not connected - WiFi WebServer not available");
  }
  
  // Setup Ethernet WebServer (Port 80) - Menggunakan EthernetServer manual
  if (Ethernet.linkStatus() == LinkON) {
    Serial.println("✅ Ethernet WebServer on port " + String(ETHERNET_WEB_SERVER_PORT));
    Serial.println("🌐 Ethernet WebServer available at: " + Ethernet.localIP().toString() + ":" + String(ETHERNET_WEB_SERVER_PORT));
    Serial.println("📡 Ethernet API access: " + Ethernet.localIP().toString() + ":" + String(ETHERNET_WEB_SERVER_PORT));
    Serial.println("🔗 Ethernet Test URL: http://" + Ethernet.localIP().toString() + ":" + String(ETHERNET_WEB_SERVER_PORT) + "/api/get/network/info");
    
    if (!ethernetServerActive) {
      Serial.println("🚀 Starting Ethernet WebServer...");
      ethernetServer.begin();
      ethernetServerActive = true;
    }
  } else {
    Serial.println("❌ Ethernet not connected - Ethernet WebServer not available");
  }
  
  Serial.println("=============================");
}

// Helper: read a line from client (ends with \r\n)
String readLine(EthernetClient &client) {
  String line = "";
  char c;
  while (client.available()) {
    c = client.read();
    line += c;
    int len = line.length();
    // check for CRLF termination
    if (len >= 2 && line.charAt(len-2) == '\r' && line.charAt(len-1) == '\n') {
      break;
    }
  }
  return line;
}

void handleEthernetServer() {
  EthernetClient client = ethernetServer.available();
  if (client) {
    Serial.println("Ethernet client connected");
    
    // Wait for data (with small timeout)
    unsigned long start = millis();
    while (!client.available() && (millis() - start) < 2000) {
      delay(1);
    }
    if (!client.available()) {
      client.stop();
      return;
    }

    // Read request line
    String requestLine = readLine(client);
    requestLine.trim();
    if (requestLine.length() == 0) {
      client.stop();
      return;
    }

    Serial.print("Ethernet Request: ");
    Serial.println(requestLine);

    // Parse method and path
    String method, path;
    int sp1 = requestLine.indexOf(' ');
    int sp2 = requestLine.indexOf(' ', sp1 + 1);
    if (sp1 > 0 && sp2 > sp1) {
      method = requestLine.substring(0, sp1);
      path = requestLine.substring(sp1 + 1, sp2);
    } else {
      client.stop();
      return;
    }

    // Read headers
    int contentLength = 0;
    while (true) {
      String header = readLine(client);
      header.trim();
      if (header.length() == 0) break; // end of headers
      
      String lower = header;
      lower.toLowerCase();
      if (lower.startsWith("content-length:")) {
        int colon = header.indexOf(':');
        if (colon >= 0) {
          String val = header.substring(colon + 1);
          val.trim();
          contentLength = val.toInt();
        }
      }
    }

    // Read body if present
    String body = "";
    if (contentLength > 0) {
      unsigned long tstart = millis();
      while ((int)body.length() < contentLength && (millis() - tstart) < 5000) {
        while (client.available() && (int)body.length() < contentLength) {
          char c = client.read();
          body += c;
        }
      }
      Serial.print("Received body (len=");
      Serial.print(body.length());
      Serial.println("):");
      Serial.println(body);
    }

    // Handle API endpoints
    handleEthernetAPI(client, method, path, body);
    
    delay(1);
    client.stop();
    Serial.println("Ethernet client disconnected");
  }
}

void handleEthernetAPI(EthernetClient &client, String method, String path, String body) {
  String response = "";
  
  // Root endpoint
  if (method == "GET" && path == "/") {
    response = "{\"message\":\"Welcome to Nearon Environment Extender Device\",";
    response += "\"clientSource\":\"Ethernet\",";
    response += "\"ethernetIP\":\"" + Ethernet.localIP().toString() + "\"";
    response += "}";
    
  } else if (method == "GET" && path == "/api/get/network/info") {
    // Network info endpoint
    response = "{\"status\":\"Success\",\"message\":\"Request Network Information Successfully\",";
    response += "\"data\":{";
    response += "\"wireless\":{\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",\"ip\":\"" + WiFi.localIP().toString() + "\",\"ssid\":\"" + WiFi.SSID() + "\",\"rssi\":" + String(WiFi.RSSI()) + "},";
    response += "\"ethernet\":{\"connected\":" + String(Ethernet.linkStatus() == LinkON ? "true" : "false") + ",\"ip\":\"" + Ethernet.localIP().toString() + "\",\"linkStatus\":" + String(Ethernet.linkStatus()) + "}";
    response += "}}";
    
  } else if (method == "GET" && path == "/api/get/sensor/data") {
    // Sensor data endpoint
    response = "{\"status\":\"Success\",\"message\":\"Request Sensor Information Successfully\",";
    response += "\"data\":{";
    for (int port = 1; port <= 8; port++) {
      String portKey = "port" + String(port);
      response += "\"" + portKey + "\":{";
      response += "\"type\":\"" + PORT_CONFIGURATED.PORT_SENSOR[port - 1] + "\",";
      response += "\"value\":{";
      response += "\"temp\":\"" + VALUE.PORT_TEMP[port - 1] + "\",";
      response += "\"humid\":\"" + VALUE.PORT_HUMID[port - 1] + "\",";
      response += "\"smoke\":\"" + VALUE.smoke_val[port - 1] + "\",";
      response += "\"door\":\"" + VALUE.door_val[port - 1] + "\",";
      response += "\"waterleak\":\"" + VALUE.waterleak_val[port - 1] + "\",";
      response += "\"servo_angle\":\"" + VALUE.servo_angle[port - 1] + "\"";
      response += "},";
      response += "\"state\":\"" + VALUE.State_port[port - 1] + "\"";
      response += "}";
      if (port < 8) response += ",";
    }
    response += "}}";
    
  } else if (method == "GET" && path == "/api/get/port/config") {
    // Port config endpoint
    response = "{\"status\":\"Success\",\"message\":\"Request Port Configuration Successfully\",";
    response += "\"data\":{";
    for (int port = 1; port <= 8; port++) {
      String portKey = "port" + String(port);
      response += "\"" + portKey + "\":{";
      response += "\"sensor\":\"" + PORT_CONFIGURATED.PORT_SENSOR[port - 1] + "\",";
      response += "\"modbus\":" + String(PORT_CONFIGURATED.PORT_MODBUSID[port - 1]) + ",";
      response += "\"pin\":" + String(PORT_CONFIGURATED.PORT_PIN[port - 1]);
      response += "}";
      if (port < 8) response += ",";
    }
    response += "}}";
    
  } else if (method == "GET" && path == "/api/get/device/config") {
    // Device config endpoint
    response = "{\"status\":\"Success\",\"message\":\"Request Device Configuration Successfully\",";
    response += "\"data\":{";
    response += "\"modbus_slave\":{";
    response += "\"id\":" + String(DEVICE_CONFIGURATED.MODBUS_SLAVE_ID) + ",";
    response += "\"state\":\"" + DEVICE_CONFIGURATED.MODBUS_SLAVE_STATE + "\"";
    response += "},";
    response += "\"hostname\":\"" + DEVICE_CONFIGURATED.DEVICE_HOSTNAME + "\",";
    response += "\"name\":\"" + DEVICE_CONFIGURATED.DEVICE_NAME + "\",";
    response += "\"sn\":\"" + DEVICE_CONFIGURATED.DEVICE_SN + "\"";
    response += "}}";
    
  } else if (method == "POST" && path == "/api/set/network/wireless") {
    // Network wireless endpoint - Use existing API function
    response = handleEthernetPostRequest("/api/set/network/wireless", body);
    
  } else if (method == "POST" && path == "/api/set/network/ethernet") {
    // Network ethernet endpoint - Use existing API function
    response = handleEthernetPostRequest("/api/set/network/ethernet", body);
    
  } else if (method == "POST" && path == "/api/set/port/config") {
    // Port config endpoint - Use existing API function
    response = handleEthernetPostRequest("/api/set/port/config", body);
    
  } else if (method == "POST" && path == "/api/set/device/config") {
    // Device config endpoint - Use existing API function
    response = handleEthernetPostRequest("/api/set/device/config", body);
    
  } else if (method == "POST" && path == "/api/control/servo/trigger") {
    // Servo trigger endpoint - Use existing API function
    response = handleEthernetPostRequest("/api/control/servo/trigger", body);
    
  } else {
    // 404 Not Found
    response = "{\"status\":\"Failed\",\"message\":\"API not found\"}";
  }
  
  // Send response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(response.length());
  client.println("Connection: close");
  client.println();
  client.print(response);
}

String handleEthernetPostRequest(String path, String body) {
  String response = "";
  
  if (path == "/api/set/network/wireless") {
    // Align with WiFi: persist request to wifi_sta.json and return consistent response
    StaticJsonDocument<2048> req;
    DeserializationError error = deserializeJson(req, body);
    if (error) {
      response = String("{\"status\":\"Failed\",\"message\":\"Invalid JSON format: ") + String(error.c_str()) + "\"}";
      return response;
    }
    // Optionally trigger WiFi reconfigure on next loop
    trigger_wireless = true;
    int writeState = filemanager.writeFile(networkSTApath.c_str(), body.c_str());
    // refresh runtime config from file so values apply immediately
    readNetworkSTAConfig();
    bool persisted = (writeState == 1);
    bool pathError = (writeState == 2);
    StaticJsonDocument<512> res;
    res["status"] = (pathError ? "Failed" : "Success");
    res["message"] = (pathError ? "Path not found or is a directory"
                                  : (persisted ? "Configuration saved successfully"
                                               : "Configuration applied (not fully persisted)"));
    res["persisted"] = persisted;
    String payload; serializeJson(res, payload); response = payload;
    
  } else if (path == "/api/set/network/ethernet") {
    // Align with WiFi: persist to eth.json and trigger reconfigure
    StaticJsonDocument<2048> req;
    DeserializationError error = deserializeJson(req, body);
    if (error) {
      response = String("{\"status\":\"Failed\",\"message\":\"Invalid JSON format: ") + String(error.c_str()) + "\"}";
      return response;
    }
    trigger_to_change_eth_setup_ontrue = true;
    int writeState = filemanager.writeFile(addressConfigEth.c_str(), body.c_str());
    // refresh ethernet runtime config so values apply immediately
    readNetworkETHConfig();
    bool persisted = (writeState == 1);
    bool pathError = (writeState == 2);
    StaticJsonDocument<512> res;
    res["status"] = (pathError ? "Failed" : "Success");
    res["message"] = (pathError ? "Path not found or is a directory"
                                  : (persisted ? "Configuration saved successfully"
                                               : "Configuration applied (not fully persisted)"));
    res["persisted"] = persisted;
    String payload; serializeJson(res, payload); response = payload;
    
  } else if (path == "/api/set/port/config") {
    // Port config endpoint - align behavior with WiFi setPort
    StaticJsonDocument<2048> req;
    DeserializationError error = deserializeJson(req, body);
    if (error) {
      Serial.println("JSON Parse Error: " + String(error.c_str()));
      response = "{\"status\":\"Failed\",\"message\":\"Invalid JSON format: " + String(error.c_str()) + "\"}";
      return response;
    }
    if (!req.is<JsonObject>()) {
      response = "{\"status\":\"Failed\",\"message\":\"JSON is not an object\"}";
      return response;
    }
    // Extract and apply to RAM
    String PORT_SENSOR[8];
    int PORT_MODBUSID[8];
    int PORT_PIN[8];
    for (int i = 1; i <= 8; i++) {
      String key = "port" + String(i);
      PORT_SENSOR[i - 1] = req[key]["sensor"].as<String>();
      PORT_MODBUSID[i - 1] = req[key]["modbus"].as<int>();
      PORT_PIN[i - 1] = req[key]["pin"].as<int>();
    }
    PORT_CONFIGURATED.setConfig(PORT_SENSOR, PORT_MODBUSID, PORT_PIN);

    // Persist to LittleFS using original JSON body
    int writeState = filemanager.writeFile(configPortpath.c_str(), body.c_str());
    bool persisted = (writeState == 1);
    bool pathError = (writeState == 2);

    StaticJsonDocument<2048> resDoc;
    resDoc["status"] = (pathError ? "Failed" : "Success");
    resDoc["message"] = (pathError
      ? "Path not found or is a directory"
      : (persisted ? "Configuration saved successfully" : "Configuration applied (not fully persisted)"));
    resDoc["persisted"] = persisted;
    JsonObject resData = resDoc.createNestedObject("data");
    for (int i = 1; i <= 8; i++) {
      String key = "port" + String(i);
      resData[key] = req[key];
    }
    String payload;
    serializeJson(resDoc, payload);
    response = payload;
    
  } else if (path == "/api/set/device/config") {
    // Align with WiFi: update DEVICE_CONFIGURATED and persist to file
    StaticJsonDocument<2048> req;
    DeserializationError error = deserializeJson(req, body);
    if (error) {
      response = String("{\"status\":\"Failed\",\"message\":\"Invalid JSON format: ") + String(error.c_str()) + "\"}";
      return response;
    }
    if (req.containsKey("modbus_slave")) {
      JsonObject ms = req["modbus_slave"];
      if (ms.containsKey("id")) DEVICE_CONFIGURATED.MODBUS_SLAVE_ID = ms["id"].as<int>();
      if (ms.containsKey("state")) DEVICE_CONFIGURATED.MODBUS_SLAVE_STATE = ms["state"].as<String>();
    }
    if (req.containsKey("hostname")) DEVICE_CONFIGURATED.DEVICE_HOSTNAME = req["hostname"].as<String>();
    if (req.containsKey("name")) DEVICE_CONFIGURATED.DEVICE_NAME = req["name"].as<String>();
    if (req.containsKey("sn")) DEVICE_CONFIGURATED.DEVICE_SN = req["sn"].as<String>();

    int writeState = filemanager.writeFile(configDevicepath.c_str(), body.c_str());
    bool persisted = (writeState == 1);
    bool pathError = (writeState == 2);
    StaticJsonDocument<512> res;
    res["status"] = (pathError ? "Failed" : "Success");
    res["message"] = (pathError ? "Path not found or is a directory"
                                  : (persisted ? "Configuration saved successfully"
                                               : "Configuration applied (not fully persisted)"));
    res["persisted"] = persisted;
    String payload; serializeJson(res, payload); response = payload;
    
  } else if (path == "/api/control/servo/trigger") {
    // Align with WiFi: accept {"port":n} and trigger servo action
    StaticJsonDocument<512> req;
    DeserializationError error = deserializeJson(req, body);
    if (error) {
      response = String("{\"status\":\"Failed\",\"message\":\"Invalid JSON format: ") + String(error.c_str()) + "\"}";
      return response;
    }
    int port = req["port"].as<int>();
    if (port >= 1 && port <= 8) {
      servo_trigger_active[port - 1] = true;
      StaticJsonDocument<256> res; res["status"] = "Success"; res["message"] = "Servo trigger processed"; String payload; serializeJson(res, payload); response = payload;
    } else {
      response = "{\"status\":\"Failed\",\"message\":\"Invalid port\"}";
    }
    
  } else {
    // 404 Not Found
    response = "{\"status\":\"Failed\",\"message\":\"API not found\"}";
  }
  
  return response;
}

void setupWebServerRoutes(WebServer* server) {
  // Root endpoint
  server->on(APIrootAddr, HTTP_GET, [server]() { getroot(server); });
  server->on(APIrootAddr, HTTP_POST, [server]() { getroot(server); });

  // Network endpoints
  server->on(APIgetNetworkInfo, HTTP_GET, [server]() { getNetworkInfo(server); });
  server->on(APIsetNetwork, HTTP_POST, [server]() { setNetworkStation(server); });
  server->on(APIsetEthernet, HTTP_POST, [server]() { setNetworkEthernet(server); });

  // Sensor endpoints
  server->on(APIsetPort, HTTP_POST, [server]() { setPort(server); });
  server->on(APIgetPort, HTTP_GET, [server]() { getPort(server); });
  server->on(APIgetSensorData, HTTP_GET, [server]() { getSensorData(server); });

  // Device endpoints
  server->on(APIsetDevice, HTTP_POST, [server]() { setDeviceConfig(server); });
  server->on(APIgetDevice, HTTP_GET, [server]() { getDeviceConfig(server); });

  // Servo control
  server->on(APIservoTrigger, HTTP_POST, [server]() { servoTrigger(server); });
  
  // Network status
  server->on("/api/network/status", HTTP_GET, [server]() { handleNetworkStatus(server); });
  
  // 404 handler
  server->onNotFound([server]() { handleNotFound(server); });
}

void loop() {
  // Main system loop

  // Handle WiFi WebServer (WebServer library)
  if (wifiServerActive) {
    wifiServer.handleClient();
  }
  
  // Handle Ethernet WebServer (EthernetServer manual)
  if (ethernetServerActive) {
    handleEthernetServer();
  }
  ota.DebugHandle();
  
  // Event-based monitoring (trigger only on state change)
  static EthernetLinkStatus prevEthStatus = Unknown;
  EthernetLinkStatus curEthStatus = eth.linkStatus();
  if (curEthStatus != prevEthStatus) {
    if (curEthStatus == LinkON) {
      Serial.println("Ethernet link up - reinitializing...");
      ethBegin();
    } else if (curEthStatus == LinkOFF) {
      Serial.println("Ethernet link down");
    }
    prevEthStatus = curEthStatus;
  }

  static wl_status_t prevWifiStatus = WL_NO_SHIELD;
  wl_status_t curWifiStatus = WiFi.status();
  if (curWifiStatus != prevWifiStatus) {
    if (curWifiStatus == WL_CONNECTED) {
      Serial.println("WiFi connected");
    } else {
      Serial.println("WiFi disconnected - attempting reconnect");
      WiFiBegin();
    }
    prevWifiStatus = curWifiStatus;
  }

  // Selalu eksekusi ulang saat ada update konfigurasi WiFi (tanpa menunggu perubahan status)
  if (trigger_wireless == 1) {
    WiFiBegin();
  }

  // Handler WiFi non-blocking untuk retry yang diperlukan
  WiFiHandle();
  updateServoMovement();  // Update servo smooth movement
  vTaskDelay(pdMS_TO_TICKS(100));
}

void updateServoMovement() {
  unsigned long currentTime = millis();

  for (int i = 0; i < 8; i++) {
    // Only process servo ports
    if (PORT_CONFIGURATED.PORT_SENSOR[i].equalsIgnoreCase("servo")) {

      // Check if servo trigger is active
      if (servo_trigger_active[i]) {
        executePushAction(i, currentTime);
      }
    }
  }
}

void executeSpeedTest(int portIndex, unsigned long currentTime) {
  static int testPhase[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  static unsigned long phaseStart[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

  // Initialize phase start time
  if (testPhase[portIndex] == 0) {
    phaseStart[portIndex] = currentTime;
    testPhase[portIndex] = 1;
    Serial.println("Servo Port-" + String(portIndex + 1) + " Speed Test: Phase 1 - MAX SPEED");
  }

  unsigned long phaseTime = currentTime - phaseStart[portIndex];

  switch (testPhase[portIndex]) {
    case 1:  // MAX SPEED - 0° to 180° to 90°
      if (phaseTime < 1000) {
        // Move to 0° at max speed
        servo[portIndex].write(0);
        servo_current_angle[portIndex] = 0;
      }
      //  else if (phaseTime < 2000) {
      //   // Move to 180° at max speed
      //   servo[portIndex].write(180);
      //   servo_current_angle[portIndex] = 180;
      // } else if (phaseTime < 3000) {
      //   // Move to 90° at max speed
      //   servo[portIndex].write(90);
      //   servo_current_angle[portIndex] = 90;
      // }
      else {
        // Phase 1 complete, test finished
        testPhase[portIndex] = 0;
        servo_trigger_active[portIndex] = false;
        VALUE.State_port[portIndex] = "Success";
        VALUE.setMsg("Speed test sequence completed", portIndex + 1);
        Serial.println("Servo Port-" + String(portIndex + 1) + " Speed Test: COMPLETED");
      }
      break;
  }

  // Update display
  VALUE.servo_angle[portIndex] = String(servo_current_angle[portIndex]);
}
void executePushAction(int portIndex, unsigned long currentTime) {
  static int pushPhase[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  static unsigned long phaseStart[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  static bool commandSent[8] = { false, false, false, false, false, false, false, false };

  // Initialize push action only if not already running
  if (pushPhase[portIndex] == 0 && servo_trigger_active[portIndex]) {
    phaseStart[portIndex] = currentTime;
    pushPhase[portIndex] = 1;
    commandSent[portIndex] = false;
    Serial.println("Servo Port-" + String(portIndex + 1) + " Push Action: Starting - Moving to PUSH position (90°)");
  }

  // Exit if trigger is not active
  if (!servo_trigger_active[portIndex]) {
    return;
  }

  unsigned long phaseTime = currentTime - phaseStart[portIndex];

  switch (pushPhase[portIndex]) {

    case 1:  // PUSH PHASE - Spin for a short duration
      if (!commandSent[portIndex]) {
        servo[portIndex].write(SERVO_PUSH_SPEED); // spin
        servo_current_angle[portIndex] = SERVO_PUSH_SPEED;
        commandSent[portIndex] = true;
        Serial.println("Servo Port-" + String(portIndex + 1) + ": Spinning for push (speed pulse=" + String(SERVO_PUSH_SPEED) + ")");
      }

      if (phaseTime >= SERVO_PUSH_MS - SERVO_DECEL_MS) {
        // begin deceleration ramp in last SERVO_DECEL_MS of push
        unsigned long decelElapsed = phaseTime - (SERVO_PUSH_MS - SERVO_DECEL_MS);
        int step = (int)((decelElapsed * SERVO_DECEL_STEPS) / (SERVO_DECEL_MS == 0 ? 1 : SERVO_DECEL_MS));
        if (step < 0) step = 0; if (step > SERVO_DECEL_STEPS) step = SERVO_DECEL_STEPS;
        int targetPulse = SERVO_PUSH_SPEED + ((SERVO_STOP_PULSE - SERVO_PUSH_SPEED) * step) / SERVO_DECEL_STEPS;
        servo[portIndex].write(targetPulse);
        servo_current_angle[portIndex] = targetPulse;
      }

      if (phaseTime >= SERVO_PUSH_MS) {
        pushPhase[portIndex] = 2;  // Move to braking phase
        phaseStart[portIndex] = currentTime;  // Reset timer for next phase
        commandSent[portIndex] = false;  // Reset command flag
        Serial.println("Servo Port-" + String(portIndex + 1) + ": Push duration done, applying brake");
      }
      break;

    case 2:  // BRAKE PHASE - brief reverse to counter inertia
      if (!commandSent[portIndex]) {
        servo[portIndex].write(SERVO_BRAKE_SPEED); // reverse briefly
        servo_current_angle[portIndex] = SERVO_BRAKE_SPEED;
        commandSent[portIndex] = true;
        Serial.println("Servo Port-" + String(portIndex + 1) + ": Braking (pulse=" + String(SERVO_BRAKE_SPEED) + ")");
      }

      if (phaseTime >= SERVO_BRAKE_MS) {
        pushPhase[portIndex] = 3;  // Move to stop phase
        phaseStart[portIndex] = currentTime;
        commandSent[portIndex] = false;
      }
      break;

    case 3:  // STOP PHASE - Stop servo and complete
      if (!commandSent[portIndex]) {
        servo[portIndex].write(SERVO_STOP_PULSE); // stop
        servo_current_angle[portIndex] = SERVO_STOP_PULSE;
        commandSent[portIndex] = true;
        Serial.println("Servo Port-" + String(portIndex + 1) + ": Stopping (pulse=" + String(SERVO_STOP_PULSE) + ")");
      }

      if (phaseTime >= SERVO_SETTLE_MS) {
        // Push action complete
        pushPhase[portIndex] = 0;  // Reset phase
        commandSent[portIndex] = false;  // Reset command flag
        servo_trigger_active[portIndex] = false;  // Deactivate trigger
        VALUE.State_port[portIndex] = "Success";
        VALUE.setMsg("Push action completed successfully", portIndex + 1);
        Serial.println("Servo Port-" + String(portIndex + 1) + " Push Action: COMPLETED - Ready for next action");
      }
      break;
  }

  // Update display with current servo angle
  VALUE.servo_angle[portIndex] = String(servo_current_angle[portIndex]);
}