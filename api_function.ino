void getroot(WebServer* server) {
  Serial.println("\nAPI Endpoint : GET or POST / HIT\n");
  
  String clientIP = server->client().remoteIP().toString();
  String response = "{\"message\":\"Welcome to Nearon Environment Extender Device\",";
  response += "\"clientIP\":\"" + clientIP + "\",";
  
  // Detect client source
  if (isWiFiClient(clientIP)) {
    response += "\"clientSource\":\"WiFi\",";
    response += "\"wifiIP\":\"" + WiFi.localIP().toString() + "\",";
    response += "\"recommendedIP\":\"" + WiFi.localIP().toString() + "\"";
  } else if (isEthernetClient(clientIP)) {
    response += "\"clientSource\":\"Ethernet\",";
    response += "\"ethernetIP\":\"" + Ethernet.localIP().toString() + "\",";
    response += "\"recommendedIP\":\"" + Ethernet.localIP().toString() + "\"";
  } else {
    response += "\"clientSource\":\"Unknown\",";
    response += "\"recommendedIP\":\"Check network status\"";
  }
  
  response += "}";
  server->send(200, "application/json", response);
}

void handleNotFound(WebServer* server) {
  String message = "API ";
  message += server->uri();
  message += " with method ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += " not found";
  for (uint8_t i = 0; i < server->args(); i++) {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  StaticJsonDocument<200> doc;
  doc["status"] = "Failed";
  doc["message"] = message;
  String data_message;
  serializeJson(doc, data_message);
  server->send(404, "application/json", data_message);
}

void getSensorData(WebServer* server) {
  Serial.println("\nAPI Endpoint : GET " + APIgetSensorData + " HIT");
  StaticJsonDocument<1024> doc;

  doc["status"] = "Success";
  doc["message"] = "Request Sensor Information Successfully";

  for (int port = 1; port <= 8; port++) {
    String portKey = "port" + String(port);
    doc["data"][portKey]["type"] = PORT_CONFIGURATED.PORT_SENSOR[port - 1];
    JsonObject value = doc["data"][portKey]["value"].to<JsonObject>();
    value["temp"] = VALUE.PORT_TEMP[port - 1];
    value["humid"] = VALUE.PORT_HUMID[port - 1];
    value["smoke"] = VALUE.smoke_val[port - 1];
    value["door"] = VALUE.door_val[port - 1];
    value["waterleak"] = VALUE.waterleak_val[port - 1];
    value["servo_angle"] = VALUE.servo_angle[port - 1];
    doc["data"][portKey]["state"] = VALUE.State_port[port - 1];
  }
  String payload;
  serializeJson(doc, payload);
  server->send(200, "application/json", payload);
  Serial.println("API Response :" + payload);
}

void getPort(WebServer* server) {
  Serial.println("\nAPI Endpoint : GET " + APIgetPort + " HIT");
  StaticJsonDocument<1024> doc;

  doc["status"] = "Success";
  doc["message"] = "Request Port Configuration Successfully";

  for (int port = 1; port <= 8; port++) {
    String portKey = "port" + String(port);
    doc["data"][portKey]["sensor"] = PORT_CONFIGURATED.PORT_SENSOR[port - 1];
    doc["data"][portKey]["modbus"] = PORT_CONFIGURATED.PORT_MODBUSID[port - 1];
    doc["data"][portKey]["pin"] = PORT_CONFIGURATED.PORT_PIN[port - 1];
  }
  String payload;
  serializeJson(doc, payload);
  server->send(200, "application/json", payload);
  Serial.println("API Response :" + payload);
}

void setPort(WebServer* server) {
  String data = server->arg("plain");
  Serial.println("\nAPI Endpoint : POST " + APIsetPort + " HIT \nAPI Body :" + data);

  // Parse input JSON
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, data);

  //---------------------INI CODE TAMBAHAN UNTUK MEMVALIDASI PIN MODE SECARA EKPLISIT (TIDAK DEFAULT) AGAR SESUAI DENGAN SENSOR---------------------------------------------------------------------
  // for (int i = 1; i <= 8; i++) {
  //   String portKey = "port" + String(i);
  //   if (doc.containsKey(portKey)) {
  //     int pin = doc[portKey]["pin"].as<int>();
  //     String sensor = doc[portKey]["sensor"].as<String>();

  //     if (pin > 0) {

  //       // if (sensor == "sht20" || sensor == "smoke" || sensor == "door" || sensor == "waterleak") {
  //       if (sensor == "door" || sensor == "waterleak") {
  //         pinMode(pin, INPUT);
  //       } else {
  //         pinMode(pin, OUTPUT);
  //       }
  //     }
  //   }
  // }
  //-------------------------------------------------------------------------------------------

  if (error) {
    StaticJsonDocument<2048> responseDoc;
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Invalid JSON format";
    JsonObject responseData = responseDoc.createNestedObject("data");
    for (int i = 1; i <= 8; i++) {
      String portKey = "port" + String(i);
      responseData[portKey] = doc[portKey];
    }
    String payload;
    serializeJson(responseDoc, payload);
    server->send(400, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");
    return;
  }

  // Validate each port's configuration
  const char* validSensors[] = { "disable", "sht20", "smoke", "door", "waterleak", "servo" };
  size_t validSensorCount = sizeof(validSensors) / sizeof(validSensors[0]);

  for (int i = 1; i <= 8; i++) {
    String portKey = "port" + String(i);
    if (!doc[portKey]) {
      StaticJsonDocument<2048> responseDoc;
      responseDoc["status"] = "Failed";
      responseDoc["message"] = "Missing configuration for " + portKey;
      JsonObject responseData = responseDoc.createNestedObject("data");
      for (int j = 1; j <= 8; j++) {
        String portKeyNested = "port" + String(j);

        responseData[portKeyNested] = doc[portKeyNested];
      }
      String payload;
      serializeJson(responseDoc, payload);
      server->send(400, "application/json", payload);
      Serial.println("API Response :" + payload + "\n");
      return;
    }


    // Validate sensor
    String sensor = doc[portKey]["sensor"].as<String>();
    sensor.toLowerCase();
    bool isSensorValid = false;
    for (size_t j = 0; j < validSensorCount; j++) {
      if (sensor == validSensors[j]) {
        isSensorValid = true;
        break;
      }
    }

    if (!isSensorValid) {
      StaticJsonDocument<2048> responseDoc;
      responseDoc["status"] = "Failed";
      responseDoc["message"] = "Invalid sensor value in " + portKey;
      JsonObject responseData = responseDoc.createNestedObject("data");
      for (int j = 1; j <= 8; j++) {
        String portKeyNested = "port" + String(j);
        responseData[portKeyNested] = doc[portKeyNested];
      }
      String payload;
      serializeJson(responseDoc, payload);
      server->send(400, "application/json", payload);
      Serial.println("API Response :" + payload + "\n");
      return;
    }
    //    sensor.toLowerCase();

    // Validate modbus for SHT20 sensor
    int modbus = doc[portKey]["modbus"].as<int>();
    if (modbus <= 0) {
      StaticJsonDocument<2048> responseDoc;
      responseDoc["status"] = "Failed";
      responseDoc["message"] = "Modbus value must be greater than 0 in " + portKey;
      JsonObject responseData = responseDoc.createNestedObject("data");
      for (int j = 1; j <= 8; j++) {
        String portKeyNested = "port" + String(j);
        responseData[portKeyNested] = doc[portKeyNested];
      }
      String payload;
      serializeJson(responseDoc, payload);
      server->send(400, "application/json", payload);
      Serial.println("API Response :" + payload + "\n");
      return;
    }

    // Validate pin for other sensors
    int pin = doc[portKey]["pin"].as<int>();
    if (pin <= 0) {
      StaticJsonDocument<2048> responseDoc;
      responseDoc["status"] = "Failed";
      responseDoc["message"] = "Pin value must be greater than 0 in " + portKey;
      JsonObject responseData = responseDoc.createNestedObject("data");
      for (int j = 1; j <= 8; j++) {
        String portKeyNested = "port" + String(j);
        responseData[portKeyNested] = doc[portKeyNested];
      }
      String payload;
      serializeJson(responseDoc, payload);
      server->send(400, "application/json", payload);
      Serial.println("API Response :" + payload + "\n");
      return;
    }
  }

  StaticJsonDocument<2048> responseDoc;
  responseDoc["status"] = "Success";
  responseDoc["message"] = "All parameters are correct";
  deserializeJson(responseDoc["data"], data);

  String PORT_SENSOR[8];
  int PORT_MODBUSID[8];
  int PORT_PIN[8];

  for (int i = 1; i <= 8; i++) {
    String portKey = "port" + String(i);
    PORT_SENSOR[i - 1] = responseDoc["data"][portKey]["sensor"].as<String>();
    PORT_MODBUSID[i - 1] = responseDoc["data"][portKey]["modbus"].as<int>();
    PORT_PIN[i - 1] = responseDoc["data"][portKey]["pin"].as<int>();
  }

  PORT_CONFIGURATED.setConfig(PORT_SENSOR, PORT_MODBUSID, PORT_PIN);

  // If all validations pass, save the configuration
  int writeState = filemanager.writeFile(configPortpath.c_str(), data.c_str());

  // Build consistent response (reuse responseDoc defined earlier)
  responseDoc.clear();
  bool persisted = (writeState == 1);
  bool pathError = (writeState == 2);

  responseDoc["status"] = (pathError ? "Failed" : "Success");
  responseDoc["message"] = (pathError
    ? "Path not found or is a directory"
    : (persisted ? "Configuration saved successfully" : "Configuration applied (not fully persisted)"));
  responseDoc["persisted"] = persisted;

  JsonObject responseData = responseDoc.createNestedObject("data");
  for (int i = 1; i <= 8; i++) {
    String portKey = "port" + String(i);
    responseData[portKey] = doc[portKey];
  }

  String payload;
  serializeJson(responseDoc, payload);
  server->send(pathError ? 500 : 200, "application/json", payload);
  Serial.println("API Response :" + payload + "\n");
  trigger_pause_other_core = true;
  triggerTochangeSettingPort = true;
}

void getDeviceConfig(WebServer* server) {
  Serial.println("\nAPI Endpoint : GET " + APIsetDevice + " HIT");
  StaticJsonDocument<1024> doc;

  doc["status"] = "Success";
  doc["message"] = "Request Device Configuration Successfully";
  doc["data"]["modbus_slave"]["id"] = DEVICE_CONFIGURATED.MODBUS_SLAVE_ID;
  doc["data"]["modbus_slave"]["state"] = DEVICE_CONFIGURATED.MODBUS_SLAVE_STATE;
  doc["data"]["hostname"] = DEVICE_CONFIGURATED.DEVICE_HOSTNAME;
  doc["data"]["name"] = DEVICE_CONFIGURATED.DEVICE_NAME;
  doc["data"]["sn"] = DEVICE_CONFIGURATED.DEVICE_SN;


  String payload;
  serializeJson(doc, payload);
  server->send(200, "application/json", payload);
  Serial.println("API Response :" + payload);
}

void setDeviceConfig(WebServer* server) {
  String data = server->arg("plain");
  Serial.println("\nAPI Endpoint : POST " + APIgetDevice + " HIT \nAPI Body :" + data);

  // Parse input JSON
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    StaticJsonDocument<2048> responseDoc;
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Invalid JSON format";
    JsonObject responseData = responseDoc.createNestedObject("data");

    responseData["modbus_slave"]["id"] = doc["modbus_slave"]["id"];
    responseData["modbus_slave"]["state"] = doc["modbus_slave"]["state"];
    responseData["hostname"] = doc["hostname"];
    responseData["name"] = doc["name"];
    responseData["sn"] = doc["sn"];

    String payload;
    serializeJson(responseDoc, payload);
    server->send(400, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");
    return;
  }

  int modbus = doc["modbus_slave"]["id"].as<int>();
  if (modbus < 1) {
    StaticJsonDocument<2048> responseDoc;
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Modbus slave id must be greater than 0";
    JsonObject responseData = responseDoc.createNestedObject("data");
    responseData["modbus_slave"]["id"] = doc["modbus_slave"]["id"];
    responseData["modbus_slave"]["state"] = doc["modbus_slave"]["state"];
    responseData["hostname"] = doc["hostname"];
    responseData["name"] = doc["name"];
    responseData["sn"] = doc["sn"];
    String payload;
    serializeJson(responseDoc, payload);
    server->send(400, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");
    return;
  }

  String state = doc["modbus_slave"]["state"].as<String>();
  state.toLowerCase();
  if (state != "enable" && state != "disable") {
    StaticJsonDocument<2048> responseDoc;
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Modbus slave state invalid, must be enable/disable";
    JsonObject responseData = responseDoc.createNestedObject("data");
    responseData["modbus_slave"]["id"] = doc["modbus_slave"]["id"];
    responseData["modbus_slave"]["state"] = doc["modbus_slave"]["state"];
    responseData["hostname"] = doc["hostname"];
    responseData["name"] = doc["name"];
    responseData["sn"] = doc["sn"];
    String payload;
    serializeJson(responseDoc, payload);
    server->send(400, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");
    return;
  }

  StaticJsonDocument<2048> responseDoc;
  responseDoc["status"] = "Success";
  responseDoc["message"] = "All parameters are correct";
  deserializeJson(responseDoc["data"], data);

  DEVICE_CONFIGURATED.setConfig(responseDoc["data"]["modbus_slave"]["id"],
                                responseDoc["data"]["modbus_slave"]["state"],
                                responseDoc["data"]["hostname"],
                                responseDoc["data"]["name"],
                                responseDoc["data"]["sn"]);

  int writeState = filemanager.writeFile(configDevicepath.c_str(), data.c_str());

  if (writeState == 1) {
    responseDoc["status"] = "Success";
    responseDoc["message"] = "Configuration saved successfully";
    JsonObject responseData = responseDoc.createNestedObject("data");
    responseData["modbus_slave"]["id"] = doc["modbus_slave"]["id"];
    responseData["modbus_slave"]["state"] = doc["modbus_slave"]["state"];
    responseData["hostname"] = doc["hostname"];
    responseData["name"] = doc["name"];
    responseData["sn"] = doc["sn"];
  } else if (writeState == 2) {
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Path not found or is a directory";
    JsonObject responseData = responseDoc.createNestedObject("data");
    responseData["modbus_slave"]["id"] = doc["modbus_slave"]["id"];
    responseData["modbus_slave"]["state"] = doc["modbus_slave"]["state"];
    responseData["hostname"] = doc["hostname"];
    responseData["name"] = doc["name"];
    responseData["sn"] = doc["sn"];
  } else {
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Failed to write configuration";
    JsonObject responseData = responseDoc.createNestedObject("data");
    responseData["modbus_slave"]["id"] = doc["modbus_slave"]["id"];
    responseData["modbus_slave"]["state"] = doc["modbus_slave"]["state"];
    responseData["hostname"] = doc["hostname"];
    responseData["name"] = doc["name"];
    responseData["sn"] = doc["sn"];
  }

  String payload;
  serializeJson(responseDoc, payload);
  server->send(writeState == 1 ? 200 : 500, "application/json", payload);
  Serial.println("API Response :" + payload + "\n");
  trigger_pause_other_core = true;
  triggerTochangeSettingPort = true;
}

void setNetworkStation(WebServer* server) {
  String data_string = server->arg("plain");
  StaticJsonDocument<384> req;
  DeserializationError error = deserializeJson(req, data_string);
  String output;

  if (error) {
    server->send(400, "application/json", "{\"status\":\"Failed\",\"message\":\"not a Valid json\"}");
    return;
  }

  if (req["network"]["mode"].as<String>().equalsIgnoreCase("dhcp")) {  // DHCP
    if (!req["ssid"].as<String>().isEmpty()) {
      dhcp_wireless = true;
      ssid_station = req["ssid"].as<String>();
      password_station = req["password"].as<String>();
      req["mode"] = req["network"]["mode"];
      req["ip"] = "";
      req["subnet"] = "";
      req["gateway"] = "";
      req["dns"] = "";
      req.remove("network");
      serializeJson(req, output);

      Serial.println("outpu 264t : " + output);
      if (!filemanager.writeFile(networkSTApath.c_str(), output.c_str())) {
        Serial.println("failed to write data network config");
      }

      output = "";
      req["status"] = "Success";
      req["message"] = "Network configuration will applied soon";
      req["data"]["ssid"] = req["ssid"];
      req["data"]["password"] = req["password"];
      req["data"]["network"]["mode"] = req["mode"];
      req["data"]["network"]["ip"] = req["ip"];
      req["data"]["network"]["subnet"] = req["subnet"];
      req["data"]["network"]["gateway"] = req["gateway"];
      req["data"]["network"]["dns"] = req["dns"];
      req.remove("mode");
      req.remove("ip");
      req.remove("subnet");
      req.remove("gateway");
      req.remove("dns");
      req.remove("ssid");
      req.remove("password");
      serializeJson(req, output);
      Serial.println("output : " + output);

      // Serial.println("the reply is : " + output);
      server->send(200, "application/json", output);
      delay(100);
      trigger_wireless = 1;
      return;
    } else {
      req["status"] = "failed";
      req["message"] = "ssid and password cannot be empty or exceed 32 characters";
      req["data"]["ssid"] = req["ssid"];
      req["data"]["password"] = req["password"];
      req["data"]["network"]["mode"] = req["network"]["mode"];
      req["data"]["network"]["ip"] = req["network"]["ip"];
      req["data"]["network"]["subnet"] = req["network"]["subnet"];
      req["data"]["network"]["gateway"] = req["network"]["gateway"];
      req["data"]["network"]["dns"] = req["network"]["dns"];
      req.remove("network");
      req.remove("ssid");
      req.remove("password");
      serializeJson(req, output);

      server->send(400, "application/json", output);
      Serial.println("data send error here the payload sent back\n" + output);
      delay(100);
      return;
    }
  } else if (req["network"]["mode"].as<String>().equalsIgnoreCase("static")) {
    int signal = validateinputmodestatic(
      req["network"]["ip"].as<String>(),
      req["network"]["subnet"].as<String>(),
      req["network"]["gateway"].as<String>(),
      req["network"]["dns"].as<String>(),
      req["ssid"],
      req["password"]);

    if (signal) {
      String msg = "";
      switch (signal) {  // static fails
        case 1:
          // ip gateway subnet cannot be empty
          msg = "ip, gateway, and subnet cannot be empty";
          break;

        case 2:
          // not a valid ip format "0.0.0.0"
          msg = "not a valid ip format 0.0.0.0";
          break;

        case 3:
          // not a valid subnet
          msg = "not a valid subnet";
          break;

        case 4:
          // ip and gateway are not in the same subnet
          msg = "ip and gateway are not in the same subnet";
          break;

        case 5:
          // ssid and password cannot be empty or exceed 32 characters
          msg = "ssid and password cannot be empty or exceed 32 characters";
          break;
      }

      req["status"] = "failed";
      req["message"] = msg;
      req["data"]["ssid"] = req["ssid"];
      req["data"]["password"] = req["password"];
      req["data"]["network"]["mode"] = req["network"]["mode"];
      req["data"]["network"]["ip"] = req["network"]["ip"];
      req["data"]["network"]["subnet"] = req["network"]["subnet"];
      req["data"]["network"]["gateway"] = req["network"]["gateway"];
      req["data"]["network"]["dns"] = req["network"]["dns"];
      req.remove("network");
      req.remove("ssid");
      req.remove("password");
      serializeJson(req, output);

      server->send(400, "application/json", output);
      Serial.println("data send error here the payload sent back\n" + output);
      delay(100);
      return;
    } else {
      dhcp_wireless = false;
      ssid_station = req["ssid"].as<String>();
      password_station = req["password"].as<String>();
      ip_station_conf.fromString(req["network"]["ip"].as<String>());
      gateway_station_conf.fromString(req["network"]["gateway"].as<String>());
      subnet_station_conf.fromString(req["network"]["subnet"].as<String>());
      dns1_station_conf.fromString(req["network"]["dns"].as<String>());

      StaticJsonDocument<384> doc;
      doc["ssid"] = ssid_station;
      doc["password"] = password_station;
      doc["mode"] = "static";
      doc["ip"] = req["network"]["ip"].as<String>();
      doc["gateway"] = req["network"]["gateway"].as<String>();
      doc["subnet"] = req["network"]["subnet"].as<String>();
      doc["dns"] = req["network"]["dns"].as<String>();
      serializeJson(doc, output);
      if (!filemanager.writeFile(networkSTApath.c_str(), output.c_str())) {
        Serial.println("failed to write data network config");
      }

      req["status"] = "Success";
      req["message"] = "Network configuration will applied soon";
      req["data"]["ssid"] = req["ssid"];
      req["data"]["password"] = req["password"];
      req["data"]["network"]["mode"] = req["network"]["mode"];
      req["data"]["network"]["ip"] = req["network"]["ip"];
      req["data"]["network"]["subnet"] = req["network"]["subnet"];
      req["data"]["network"]["gateway"] = req["network"]["gateway"];
      req["data"]["network"]["dns"] = req["network"]["dns"];
      req.remove("network");
      req.remove("ssid");
      req.remove("password");
      serializeJson(req, output);

      server->send(200, "application/json", output);
      trigger_wireless = 1;
      delay(100);
      return;
    }
  } else {

    req["status"] = "failed";
    req["message"] = "mode must be filled cannot be empty and only dhcp xor static";
    req["data"]["ssid"] = req["ssid"];
    req["data"]["password"] = req["password"];
    req["data"]["network"]["mode"] = req["network"]["mode"];
    req["data"]["network"]["ip"] = req["network"]["ip"];
    req["data"]["network"]["subnet"] = req["network"]["subnet"];
    req["data"]["network"]["gateway"] = req["network"]["gateway"];
    req["data"]["network"]["dns"] = req["network"]["dns"];
    req.remove("ssid");
    req.remove("password");
    req.remove("network");
    serializeJson(req, output);
    server->send(400, "application/json", output);
  }
}

void getNetworkInfo(WebServer* server) {
  WiFiState();
  StaticJsonDocument<20> doc;

  String modewifi;
  if (dual_wifi == 1) {
    modewifi = "AP Mode";
  } else {
    modewifi = "Dual Mode";
  }

  String network_setting = filemanager.readFile(networkSTApath.c_str());
  StaticJsonDocument<384> boo;
  DeserializationError error = deserializeJson(boo, network_setting);

  String tempMode, tempip, tempsubnet, tempgateway, tempdns;
  if (boo["mode"].as<String>().equalsIgnoreCase("") || !boo.containsKey("mode")) {
    tempMode = "";
  } else {
    tempMode = boo["mode"].as<String>();
  }
  if (boo["ip"].as<String>().equalsIgnoreCase("") || !boo.containsKey("ip")) {
    tempip = "";
  } else {
    tempip = boo["ip"].as<String>();
  }
  if (boo["subnet"].as<String>().equalsIgnoreCase("") || !boo.containsKey("subnet")) {
    tempsubnet = "";
  } else {
    tempsubnet = boo["subnet"].as<String>();
  }
  if (boo["gateway"].as<String>().equalsIgnoreCase("") || !boo.containsKey("gateway")) {
    tempgateway = "";
  } else {
    tempgateway = boo["gateway"].as<String>();
  }
  if (boo["dns"].as<String>().equalsIgnoreCase("") || !boo.containsKey("dns")) {
    tempdns = "";
  } else {
    tempdns = boo["dns"].as<String>();
  }

  doc["status"] = "Success";
  doc["message"] = "Request Network Information Successfully";
  doc["data"]["wireless"]["mode"] = modewifi;
  doc["data"]["wireless"]["ap"]["mac"] = mac_ap;
  doc["data"]["wireless"]["ap"]["ip"] = ip_ap;
  doc["data"]["wireless"]["ap"]["ssid"] = ssid_ap;
  doc["data"]["wireless"]["ap"]["password"] = password_ap;
  doc["data"]["wireless"]["station"]["mac"] = mac_station;
  doc["data"]["wireless"]["station"]["ip"] = ip_station;
  doc["data"]["wireless"]["station"]["ssid"] = ssid_station;
  doc["data"]["wireless"]["station"]["password"] = password_station;
  doc["data"]["wireless"]["station"]["rssi"] = wifi_rssi;
  doc["data"]["wireless"]["station"]["state"] = String(wifi_status);
  doc["data"]["wireless"]["station"]["configuration"]["mode"] = tempMode;
  doc["data"]["wireless"]["station"]["configuration"]["ip"] = ip_station_conf.toString();
  doc["data"]["wireless"]["station"]["configuration"]["subnet"] = subnet_station_conf.toString();
  doc["data"]["wireless"]["station"]["configuration"]["gateway"] = gateway_station_conf.toString();
  doc["data"]["wireless"]["station"]["configuration"]["dns"] = dns1_eth_conf.toString();

  doc["data"]["ethernet"]["mode"] = dhcp_eth ? "dhcp" : "static";
  doc["data"]["ethernet"]["mac"] = macETH;
  doc["data"]["ethernet"]["ip"] = ip_eth_conf.toString();
  doc["data"]["ethernet"]["subnet"] = subnet_eth_conf.toString();
  doc["data"]["ethernet"]["gateway"] = gateway_eth_conf.toString();
  doc["data"]["ethernet"]["dns"] = dns1_eth_conf.toString();

  String data_string;
  serializeJson(doc, data_string);
  Serial.println("--- Request GET Information Network ---");
  Serial.println(data_string);
  Serial.println("---------------------------------------\n");
  server->send(200, "text/json", data_string);
}

void setNetworkEthernet(WebServer* server) {
  String data_string = server->arg("plain");
  Serial.println("data received :" + data_string);
  StaticJsonDocument<256> req;
  DeserializationError error = deserializeJson(req, data_string);
  String output;
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    server->send(400, "application/json", "{\"status\": \"failed\",\"messege\": \"please provide with a valid json format\"}");
    Serial.println("return this: \"messege\":\"plese provide with a valid json format\"");
    return;
  }

  if (req["mode"].as<String>().equalsIgnoreCase("static")) {
    Serial.println(req["ip"].as<String>());
    Serial.println(req["subnet"].as<String>());
    Serial.println(req["gateway"].as<String>());
    Serial.println(req["dns"].as<String>());
    int result = validateInputforEthernet(req["ip"].as<String>(), req["subnet"].as<String>(), req["gateway"].as<String>(), req["dns"].as<String>());
    Serial.println(String(result));

    if (result) {  // static fails
      String msg = "";
      Serial.println(result);
      switch (result) {
        case 1:
          msg = "ip , subnet or gateway cannot be empty at static mode";
          break;

        case 2:
          msg = "you ip format is unknown, String represent IPAddress 1.1.1.1";
          break;

        case 3:
          msg = "your subnet is not correct";
          break;

        case 4:
          msg = "ip and gateway must be in the same subnet";
          break;
      }

      req["status"] = "failed";
      req["message"] = msg;
      req["data"]["mode"] = req["mode"];
      req["data"]["ip"] = req["ip"];
      req["data"]["subnet"] = req["subnet"];
      req["data"]["gateway"] = req["gateway"];
      req["data"]["dns"] = req["dns"];
      req.remove("ip");
      req.remove("subnet");
      req.remove("mode");
      req.remove("gateway");
      req.remove("dns");
      serializeJson(req, output);

      server->send(400, "application/json", output);
      Serial.println("resposnse msg  :" + output);
      return;
    } else if (!result) {  // static good
      serializeJson(req, output);
      if (!filemanager.writeFile(addressConfigEth.c_str(), output.c_str())) {
        Serial.println("failed to write data network config");
      }

      Serial.println("written to flash:" + output);
      output = "";
      req["status"] = "Success";
      req["message"] = "setup eth will be applied soon";
      req["data"]["mode"] = req["mode"];
      req["data"]["ip"] = req["ip"];
      req["data"]["subnet"] = req["subnet"];
      req["data"]["gateway"] = req["gateway"];
      req["data"]["dns"] = req["dns"];
      req.remove("ip");
      req.remove("subnet");
      req.remove("mode");
      req.remove("gateway");
      req.remove("dns");
      serializeJson(req, output);

      server->send(200, "application/json", output);
      Serial.println("resposnse msg  :" + output);
      dhcp_eth = false;
      trigger_to_change_eth_setup_ontrue = true;
      return;
    }
  } else if (req["mode"].as<String>().equalsIgnoreCase("dhcp")) {
    req["ip"] = "";
    req["subnet"] = "";
    req["gateway"] = "";
    req["dns"] = "";
    serializeJson(req, output);
    if (!filemanager.writeFile(addressConfigEth.c_str(), output.c_str())) {
      Serial.println("failed to write data network config");
    }
    Serial.println(output);
    req["status"] = "Success";
    req["message"] = "setup eth will be applied soon";
    req["data"]["mode"] = "dhcp";
    req["data"]["ip"] = req["ip"];
    req["data"]["subnet"] = req["subnet"];
    req["data"]["gateway"] = req["gateway"];
    req["data"]["dns"] = req["dns"];
    req.remove("mode");
    req.remove("ip");
    req.remove("subnet");
    req.remove("gateway");
    req.remove("dns");
    serializeJson(req, output);

    server->send(200, "application/json", output);
    Serial.println("resposnse output  :" + output);
    dhcp_eth = true;
    trigger_to_change_eth_setup_ontrue = true;
    return;
  } else {
    req["status"] = "failed";
    req["message"] = "wrong mode, just acccept dhcp xor static string key insensitive";
    req["data"]["mode"] = req["mode"];
    req["data"]["ip"] = req["ip"];
    req["data"]["subnet"] = req["subnet"];
    req["data"]["gateway"] = req["gateway"];
    req["data"]["dns"] = req["dns"];
    req.remove("ip");
    req.remove("mode");
    req.remove("subnet");
    req.remove("gateway");
    req.remove("dns");
    serializeJson(req, output);

    server->send(400, "application/json", output);
    Serial.println("return this: " + output);
    return;
  }
}

void servoTrigger(WebServer* server) {
  String data = server->arg("plain");
  Serial.println("\nAPI Endpoint : POST " + APIservoTrigger + " HIT \nAPI Body :" + data);

  // Parse input JSON
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    StaticJsonDocument<512> responseDoc;
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Invalid JSON format";
    String payload;
    serializeJson(responseDoc, payload);
    server->send(400, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");
    return;
  }

  // Validate required fields
  if (!doc.containsKey("port") || !doc.containsKey("trigger")) {
    StaticJsonDocument<512> responseDoc;
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Missing required fields: port and trigger";
    String payload;
    serializeJson(responseDoc, payload);
    server->send(400, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");
    return;
  }

  int port = doc["port"].as<int>();
  int trigger = doc["trigger"].as<int>();

  // Validate port range
  if (port < 1 || port > 8) {
    StaticJsonDocument<512> responseDoc;
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Port must be between 1 and 8";
    String payload;
    serializeJson(responseDoc, payload);
    server->send(400, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");
    return;
  }

  // Validate trigger value (0 = OFF, 1 = ON)
  if (trigger != 0 && trigger != 1) {
    StaticJsonDocument<512> responseDoc;
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Trigger must be 0 (OFF) or 1 (ON)";
    String payload;
    serializeJson(responseDoc, payload);
    server->send(400, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");
    return;
  }

  // Check if port is configured as servo
  if (!PORT_CONFIGURATED.PORT_SENSOR[port - 1].equalsIgnoreCase("servo")) {
    StaticJsonDocument<512> responseDoc;
    responseDoc["status"] = "Failed";
    responseDoc["message"] = "Port " + String(port) + " is not configured as servo";
    String payload;
    serializeJson(responseDoc, payload);
    server->send(400, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");
    return;
  }

  // Handle trigger command
  if (trigger == 1) {
    // ON - Start push action sequence
    servo_trigger_active[port - 1] = true;
    servo_trigger_start[port - 1] = millis();
    VALUE.State_port[port - 1] = "Push Action";
    VALUE.setMsg("Push action sequence started", port);

    Serial.println("Servo Port-" + String(port) + " Push Action: STARTED");

    // Success response
    StaticJsonDocument<512> responseDoc;
    responseDoc["status"] = "Success";
    responseDoc["message"] = "Servo push action sequence started";
    responseDoc["data"]["port"] = port;
    responseDoc["data"]["trigger"] = "ON";
    responseDoc["data"]["action"] = "Push action sequence activated";
    responseDoc["data"]["pin"] = PORT_CONFIGURATED.PORT_PIN[port - 1];

    String payload;
    serializeJson(responseDoc, payload);
    server->send(200, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");

  } else {
    // OFF - Stop push action sequence
    servo_trigger_active[port - 1] = false;
    servo_trigger_start[port - 1] = 0;
    VALUE.State_port[port - 1] = "Success";
    VALUE.setMsg("Push action sequence stopped", port);

    // Return servo to center position
    servo[port - 1].write(90);
    servo_current_angle[port - 1] = 90;

    Serial.println("Servo Port-" + String(port) + " Speed Test: STOPPED");

    // Success response
    StaticJsonDocument<512> responseDoc;
    responseDoc["status"] = "Success";
    responseDoc["message"] = "Servo speed test sequence stopped";
    responseDoc["data"]["port"] = port;
    responseDoc["data"]["trigger"] = "OFF";
    responseDoc["data"]["action"] = "Speed test sequence deactivated";
    responseDoc["data"]["pin"] = PORT_CONFIGURATED.PORT_PIN[port - 1];

    String payload;
    serializeJson(responseDoc, payload);
    server->send(200, "application/json", payload);
    Serial.println("API Response :" + payload + "\n");
  }
}

void handleNetworkStatus(WebServer* server) {
  DynamicJsonDocument doc(1024);
  
  // WiFi status
  doc["wifi"]["connected"] = (WiFi.status() == WL_CONNECTED);
  doc["wifi"]["ip"] = WiFi.localIP().toString();
  doc["wifi"]["ssid"] = WiFi.SSID();
  doc["wifi"]["rssi"] = WiFi.RSSI();
  
  // Ethernet status
  doc["ethernet"]["connected"] = (Ethernet.linkStatus() == LinkON);
  doc["ethernet"]["ip"] = Ethernet.localIP().toString();
  doc["ethernet"]["linkStatus"] = Ethernet.linkStatus();
  
  // Primary network recommendation
  if (Ethernet.linkStatus() == LinkON) {
    doc["primary"]["type"] = "ethernet";
    doc["primary"]["ip"] = Ethernet.localIP().toString();
    doc["primary"]["message"] = "Use this IP for API access";
    doc["primary"]["testUrl"] = "http://" + Ethernet.localIP().toString() + "/api/get/network/info";
  } else if (WiFi.status() == WL_CONNECTED) {
    doc["primary"]["type"] = "wifi";
    doc["primary"]["ip"] = WiFi.localIP().toString();
    doc["primary"]["message"] = "Use this IP for API access";
    doc["primary"]["testUrl"] = "http://" + WiFi.localIP().toString() + "/api/get/network/info";
  } else {
    doc["primary"]["type"] = "none";
    doc["primary"]["ip"] = "";
    doc["primary"]["message"] = "No network connection";
  }
  
  String response;
  serializeJson(doc, response);
  server->send(200, "application/json", response);
  Serial.println("Network Status Response: " + response);
}

bool isWiFiClient(String clientIP) {
  if (WiFi.status() != WL_CONNECTED) return false;
  
  IPAddress wifiIP = WiFi.localIP();
  IPAddress clientIPAddr;
  clientIPAddr.fromString(clientIP);
  
  return (clientIPAddr[0] == wifiIP[0] && 
          clientIPAddr[1] == wifiIP[1] && 
          clientIPAddr[2] == wifiIP[2]);
}

bool isEthernetClient(String clientIP) {
  if (Ethernet.linkStatus() != LinkON) return false;
  
  IPAddress ethIP = Ethernet.localIP();
  IPAddress clientIPAddr;
  clientIPAddr.fromString(clientIP);
  
  return (clientIPAddr[0] == ethIP[0] && 
          clientIPAddr[1] == ethIP[1] && 
          clientIPAddr[2] == ethIP[2]);
}
