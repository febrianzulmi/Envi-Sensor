void readSensorTask(void *pvParameters) {
  while (true) {
    if (trigger_pause_other_core == true) {
      if (triggerTochangeSettingPort == true) {
        initializeSensor();
        triggerTochangeSettingPort = false;
      }
      trigger_pause_other_core = false;
    }
    readSensorMain();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void processSensor(uint8_t portNumber, String sensorType, uint8_t pin) {
  uint16_t r = 0;  // Variable untuk menangkap error serial modbus
  if (sensorType.equalsIgnoreCase("sht20")) {
    r = sht20[portNumber - 1].readInputRegisters(0x0001, 0x02);
    if (r == sht20[portNumber - 1].ku8MBSuccess) {
      VALUE.PORT_TEMP[portNumber - 1] = String(sht20[portNumber - 1].getResponseBuffer(0) / 10);
      VALUE.PORT_HUMID[portNumber - 1] = String(sht20[portNumber - 1].getResponseBuffer(1) / 10);
      VALUE.door_val[portNumber - 1] = "-";
      VALUE.smoke_val[portNumber - 1] = "-";
      VALUE.waterleak_val[portNumber - 1] = "-";
      VALUE.State_port[portNumber - 1] = "Success";
      VALUE.setMsg("Sensor OK", portNumber);
      countErrorSensorPort[portNumber - 1] = 0;
      Serial.println("Port-" + String(portNumber) + " (SHT20) : Temperature (" + String(VALUE.PORT_TEMP[portNumber - 1]) + ") | Humid (" + VALUE.PORT_HUMID[portNumber - 1] + ")");
    } else {
      countErrorSensorPort[portNumber - 1] += 1;
      if (countErrorSensorPort[portNumber - 1] >= 10) {
        VALUE.PORT_TEMP[portNumber - 1] = "0";
        VALUE.PORT_HUMID[portNumber - 1] = "0";
        VALUE.door_val[portNumber - 1] = "-";
        VALUE.smoke_val[portNumber - 1] = "-";
        VALUE.waterleak_val[portNumber - 1] = "-";
        VALUE.State_port[portNumber - 1] = "Error";
        VALUE.setMsg("Sensor Error with Code " + String(r, 16), portNumber);
        Serial.println("Port-" + String(portNumber) + " (SHT20) : Temperature (" + String(VALUE.PORT_TEMP[portNumber - 1]) + ") | Humid (" + VALUE.PORT_HUMID[portNumber - 1] + ") -> Error");
      } else {
        VALUE.door_val[portNumber - 1] = "-";
        VALUE.smoke_val[portNumber - 1] = "-";
        VALUE.waterleak_val[portNumber - 1] = "-";
        VALUE.State_port[portNumber - 1] = "Error Tolerance";
        VALUE.setMsg("Sensor Tolerance OK with Code " + String(r, 16), portNumber);
        Serial.println("Port-" + String(portNumber) + " (SHT20) : Temperature (" + String(VALUE.PORT_TEMP[portNumber - 1]) + ") | Humid (" + VALUE.PORT_HUMID[portNumber - 1] + ") -> Error Tolerance");
      }
    }
  } else if (sensorType.equalsIgnoreCase("door")) {
    VALUE.PORT_TEMP[portNumber - 1] = "-";
    VALUE.PORT_HUMID[portNumber - 1] = "-";
    VALUE.door_val[portNumber - 1] = String(digitalRead(pin));
    VALUE.smoke_val[portNumber - 1] = "-";
    VALUE.waterleak_val[portNumber - 1] = "-";
    VALUE.State_port[portNumber - 1] = "Success";
    VALUE.setMsg("Sensor OK", portNumber);
    Serial.println("Port-" + String(portNumber) + " (Door) : " + String(VALUE.door_val[portNumber - 1]));
  } else if (sensorType.equalsIgnoreCase("smoke")) {
    VALUE.PORT_TEMP[portNumber - 1] = "-";
    VALUE.PORT_HUMID[portNumber - 1] = "-";
    VALUE.door_val[portNumber - 1] = "-";
    VALUE.smoke_val[portNumber - 1] = String(digitalRead(pin));
    VALUE.waterleak_val[portNumber - 1] = "-";
    VALUE.State_port[portNumber - 1] = "Success";
    VALUE.setMsg("Sensor OK", portNumber);
    Serial.println("Port-" + String(portNumber) + " (Smoke) : " + String(VALUE.smoke_val[portNumber - 1]));
  } else if (sensorType.equalsIgnoreCase("waterleak")) {
    VALUE.PORT_TEMP[portNumber - 1] = "-";
    VALUE.PORT_HUMID[portNumber - 1] = "-";
    VALUE.door_val[portNumber - 1] = "-";
    VALUE.smoke_val[portNumber - 1] = "-";
    VALUE.waterleak_val[portNumber - 1] = String(digitalRead(pin));
    VALUE.State_port[portNumber - 1] = "Success";
    VALUE.setMsg("Sensor OK", portNumber);
    Serial.println("Port-" + String(portNumber) + " (WaterLeak) : " + String(VALUE.waterleak_val[portNumber - 1]));
  } else if (sensorType.equalsIgnoreCase("servo")) {
    VALUE.PORT_TEMP[portNumber - 1] = "-";
    VALUE.PORT_HUMID[portNumber - 1] = "-";
    VALUE.door_val[portNumber - 1] = "-";
    VALUE.smoke_val[portNumber - 1] = "-";
    VALUE.waterleak_val[portNumber - 1] = "-";
    // servo_angle dan servo_speed tidak diupdate di sini karena dikontrol via API
    VALUE.State_port[portNumber - 1] = "Success";
    VALUE.setMsg("Servo Ready", portNumber);
    Serial.println("Port-" + String(portNumber) + " (Servo) : Angle " + VALUE.servo_angle[portNumber - 1] + "°");
  } else if (sensorType.equalsIgnoreCase("disable")) {
    VALUE.PORT_TEMP[portNumber - 1] = "N/A";
    VALUE.PORT_HUMID[portNumber - 1] = "N/A";
    VALUE.door_val[portNumber - 1] = "N/A";
    VALUE.smoke_val[portNumber - 1] = "N/A";
    VALUE.waterleak_val[portNumber - 1] = "N/A";
    VALUE.servo_angle[portNumber - 1] = "N/A";
    VALUE.State_port[portNumber - 1] = "Disable";
    VALUE.setMsg("Sensor Port Disable", portNumber);
    Serial.println("Port-" + String(portNumber) + " (Disable) : N/A");
  } else {
    VALUE.PORT_TEMP[portNumber - 1] = "-1";
    VALUE.PORT_HUMID[portNumber - 1] = "-1";
    VALUE.door_val[portNumber - 1] = "-1";
    VALUE.smoke_val[portNumber - 1] = "-1";
    VALUE.waterleak_val[portNumber - 1] = "-1";
    VALUE.servo_angle[portNumber - 1] = "-1";
    VALUE.State_port[portNumber - 1] = "Error";
    VALUE.setMsg("Sensor/Port Error", portNumber);
    Serial.println("Port-" + String(portNumber) + " (N/A) : -1");
  }
}

void readSensorMain() {
  for (int i = 1; i <= 8; i++) {
    processSensor(i, PORT_CONFIGURATED.PORT_SENSOR[i - 1], PORT_CONFIGURATED.PORT_PIN[i - 1]);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  Serial.println("");
}



void configurePort(String sensorType, int modbusId, int pin, int portNumber) {
  if (sensorType.equalsIgnoreCase("sht20")) {
    sht20[portNumber - 1].begin(modbusId, modbusSerial);

    Serial.println("SHT20 Port " + String(portNumber) + " Enable on Modbus ID " + String(modbusId));
  } else if (sensorType.equalsIgnoreCase("door")) {
    Serial.println("Door Port " + String(portNumber) + " Enable on PIN " + String(pin));
    pinMode(pin, INPUT);
  } else if (sensorType.equalsIgnoreCase("smoke")) {
    Serial.println("Smoke Port " + String(portNumber) + " Enable on PIN " + String(pin));
    pinMode(pin, INPUT);
  } else if (sensorType.equalsIgnoreCase("waterleak")) {
    Serial.println("Water Leak Port " + String(portNumber) + " Enable on PIN " + String(pin));
    pinMode(pin, INPUT);
  } else if (sensorType.equalsIgnoreCase("servo")) {
    Serial.println("Servo Port " + String(portNumber) + " Enable on PIN " + String(pin));
    servo[portNumber - 1].attach(pin);
    servo[portNumber - 1].write(90); // Stop position for continuous servo
    VALUE.servo_angle[portNumber - 1] = "90"; // Stop
    servo_current_angle[portNumber - 1] = 90;
  } else {
    Serial.println("Port " + String(portNumber) + " Sensor Status Disable");
  }
}
void initializeSensor() {
  Serial.println("---------- Initialization Sensor ----------");
  for (int i = 0; i < 8; i++) {
    configurePort(PORT_CONFIGURATED.PORT_SENSOR[i], PORT_CONFIGURATED.PORT_MODBUSID[i], PORT_CONFIGURATED.PORT_PIN[i], i + 1);
  }
  Serial.println("-------------------------------------------\n");
}
