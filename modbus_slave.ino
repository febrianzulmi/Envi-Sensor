#include "config.h"  // ESP32 3.3.1 Configuration
#include <ModbusRTU.h> // https://github.com/emelianov/modbus-esp8266
#include <HardwareSerial.h>

ModbusRTU mb;

// Semua konfigurasi sudah didefinisikan di config.h


struct SensorData {      
    uint16_t temperature;  
    uint16_t humidity;     
    uint16_t smoke;     
    uint16_t door;
    uint16_t waterLeak; 
    uint16_t servoAngle;
    uint16_t state;     
};

struct portData {
    uint16_t type;
    uint16_t modbus_id;
    uint16_t pin;
};

struct lastPortData {
    uint16_t type;
    uint16_t modbus_id;
    uint16_t pin;
};

SensorData sensors[NUM_PORTS];

portData ports[NUM_PORTS];

lastPortData lastPorts[NUM_PORTS];

int sensorTypeToEnum(String sensorType){
    if (sensorType.equalsIgnoreCase("sht20")) {
        return 1;
    } else if (sensorType.equalsIgnoreCase("smoke")) {
        return 2;
    } else if (sensorType.equalsIgnoreCase("door")) {
        return 3;
    } else if (sensorType.equalsIgnoreCase("waterleak")) {
        return 4;
    } else if (sensorType.equalsIgnoreCase("servo")) {
        return 5;
    } else {
        return 99; // auto disable untuk disable dan unknown types
    }
}

String enumToSensorType(int data){
    switch(data){
        case 1:
            return "SHT20";
            break;
        case 2:
            return "Smoke";
            break;
        case 3:
            return "Door";
            break;
        case 4:
            return "WaterLeak";
            break;
        case 5:
            return "Servo";
            break;
        case 99:
            return "Disable";
            break;
        default:
            return "Disable"; // auto disable untuk unknown enum
            break;
    }
}

int stateToEnum(String state){
    if (state.equalsIgnoreCase("success")) {
        return 1;
    } else if (state.equalsIgnoreCase("error tolerance")) {
        return 2;
    } else if (state.equalsIgnoreCase("error")) {
        return 3;
    } else if (state.equalsIgnoreCase("disable")) {
        return 99;
    } else {
        return 0; //unknown
    }
}

void initializeModbusValue() {

    for (uint8_t i = 0; i < NUM_PORTS; i++) {
        ports[i].type = 0; 
        sensors[i].temperature = 0.0;
        sensors[i].humidity = 0.0;
        sensors[i].smoke = 0;
        sensors[i].door = 0;
        sensors[i].waterLeak = 0;
        sensors[i].servoAngle = 90;
        sensors[i].state = 0;
        ports[i].modbus_id = 0;
        ports[i].pin = 0;
    }
}

void updateSensorValues() {
    for (uint8_t i = 0; i < NUM_PORTS; i++) {
        sensors[i].temperature = (int)(round(VALUE.PORT_TEMP[i].toFloat() * 10)); 
        sensors[i].humidity = (int)(round(VALUE.PORT_HUMID[i].toFloat() * 10));
        sensors[i].smoke = VALUE.smoke_val[i].toInt();       
        sensors[i].door = VALUE.door_val[i].toInt();        
        sensors[i].waterLeak = VALUE.waterleak_val[i].toInt(); 
        sensors[i].servoAngle = VALUE.servo_angle[i].toInt();
        sensors[i].state = stateToEnum(VALUE.State_port[i]);
    }
}

void updatePortValues() {
     for (uint8_t i = 0; i < NUM_PORTS; i++) {
        ports[i].type = sensorTypeToEnum(PORT_CONFIGURATED.PORT_SENSOR[i]);
        ports[i].modbus_id = PORT_CONFIGURATED.PORT_MODBUSID[i];
        ports[i].pin = PORT_CONFIGURATED.PORT_PIN[i];
     }
}

void configureModbusRegisters() {
    for (uint8_t i = 0; i < NUM_PORTS; i++) {
        mb.addHreg(REG_TYPE_START + i, (uint16_t)ports[i].type);
        mb.addHreg(REG_VALUE_START + i * 6 + 0, sensors[i].temperature);
        mb.addHreg(REG_VALUE_START + i * 6 + 1, sensors[i].humidity);
        mb.addHreg(REG_VALUE_START + i * 6 + 2, sensors[i].smoke);
        mb.addHreg(REG_VALUE_START + i * 6 + 3, sensors[i].door);
        mb.addHreg(REG_VALUE_START + i * 6 + 4, sensors[i].waterLeak);
        mb.addHreg(REG_VALUE_START + i * 6 + 5, sensors[i].servoAngle);
        mb.addHreg(REG_STATE_START + i, (uint16_t)sensors[i].state);
        mb.addHreg(REG_PORT_MODBUS_ID_START + i, (uint16_t)ports[i].modbus_id);
        mb.addHreg(REG_PORT_PIN_START + i, (uint16_t)ports[i].pin);
    }
}

void updateModbusRegisters() {
    for (uint8_t i = 0; i < NUM_PORTS; i++) {
        // Perbarui nilai di register Modbus
        mb.Hreg(REG_TYPE_START + i, (uint16_t)ports[i].type);
        mb.Hreg(REG_VALUE_START + i * 6 + 0, sensors[i].temperature);
        mb.Hreg(REG_VALUE_START + i * 6 + 1, sensors[i].humidity);
        mb.Hreg(REG_VALUE_START + i * 6 + 2, sensors[i].smoke);
        mb.Hreg(REG_VALUE_START + i * 6 + 3, sensors[i].door);
        mb.Hreg(REG_VALUE_START + i * 6 + 4, sensors[i].waterLeak);
        mb.Hreg(REG_VALUE_START + i * 6 + 5, sensors[i].servoAngle);
        mb.Hreg(REG_STATE_START + i, (uint16_t)sensors[i].state);
        mb.Hreg(REG_PORT_MODBUS_ID_START + i, (uint16_t)ports[i].modbus_id);
        mb.Hreg(REG_PORT_PIN_START + i, (uint16_t)ports[i].pin);
    }
}

void storeLastRegister (){
    for (uint8_t i = 0; i < NUM_PORTS; i++){
        lastPorts[i].type = mb.Hreg(REG_TYPE_START + i);
        lastPorts[i].modbus_id = mb.Hreg(REG_PORT_MODBUS_ID_START + i);
        lastPorts[i].pin = mb.Hreg(REG_PORT_PIN_START + i);
    }
}

void masterWriteHandler(){
    String type[8];
    int modbus_id[8];
    int pin[8];
    bool isChanged = false;
    StaticJsonDocument<1024> doc;
    for (uint8_t i = 0; i < NUM_PORTS; i++){
        if (lastPorts[i].type != mb.Hreg(REG_TYPE_START + i)){
            type[i] = enumToSensorType(mb.Hreg(REG_TYPE_START + i));
            isChanged = true;
        }else{
            type[i] = enumToSensorType(lastPorts[i].type);
        }

        if (lastPorts[i].modbus_id != mb.Hreg(REG_PORT_MODBUS_ID_START + i)){
            modbus_id[i] = mb.Hreg(REG_PORT_MODBUS_ID_START + i);
            isChanged = true;
        }else{
            modbus_id[i] = lastPorts[i].modbus_id;
        }

        if (lastPorts[i].pin != mb.Hreg(REG_PORT_PIN_START + i)){
            pin[i] = mb.Hreg(REG_PORT_PIN_START + i);
            isChanged = true;
        }else{
            pin[i] = lastPorts[i].pin;
        }

        String portKey = "port" + String(i + 1);
        doc[portKey]["sensor"] = type[i];
        doc[portKey]["modbus"] = modbus_id[i];
        doc[portKey]["pin"] = pin[i];
    }

    if(isChanged == true){
        PORT_CONFIGURATED.setConfig(type, modbus_id, pin);
        isChanged = false;
        String data;
        serializeJson(doc, data);
        filemanager.writeFile(configPortpath.c_str(), data.c_str());
        trigger_pause_other_core = true;
        triggerTochangeSettingPort = true;
    }
}

void modbusSlaveTask(void *pvParameters) {
    Serial.println("\nModbus SLAVE STARTED\n"); 
    
    // Initialize Serial2 for ESP32 3.3.1 with proper pin configuration
    Serial2.begin(9600, SERIAL_8N1, PIN_RO, PIN_RE);
    
    // Initialize ModbusRTU with proper configuration for ESP32 3.3.1
    mb.begin(&Serial2, PIN_RE);
    mb.setBaudrate(9600);
    mb.slave(DEVICE_CONFIGURATED.MODBUS_SLAVE_ID);
    
    initializeModbusValue();
    configureModbusRegisters();
    updateSensorValues();
    updateModbusRegisters();
    
    while (true) {
        if(DEVICE_CONFIGURATED.MODBUS_SLAVE_STATE.equalsIgnoreCase("enable")){
          masterWriteHandler();
          updatePortValues();
          updateSensorValues();
          updateModbusRegisters();
          storeLastRegister();
          mb.slave(DEVICE_CONFIGURATED.MODBUS_SLAVE_ID);
          mb.task();
          vTaskDelay(pdMS_TO_TICKS(1000)); 
        } else {
          vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
}
