bool isNumberString(const String &str) {
  for (char c : str) {
    if (!isdigit(c)) {
      return false;
    }
  }
  return true;
}

// Function to validate input data
int validateinputmodestatic(String temp_network_ip, String temp_network_subnet,
                            String temp_network_gateway, String temp_network_dns, String temp_ssid,
                            String temp_password) {
  // Validate if not null
  if (temp_network_ip.equalsIgnoreCase("") || temp_network_subnet.equalsIgnoreCase("") || temp_network_gateway.equalsIgnoreCase("")) {
    return 1;
  }

  // Validate the IPs are correct
  if (!isValidIPAddress(temp_network_ip) || !isValidIPAddress(temp_network_subnet) || !isValidIPAddress(temp_network_gateway)) {
    return 2;
  }

  // Validate subnet validity
  if (!validateSubnet(temp_network_subnet)) {
    return 3;
  }

  // Validate if gateway and IP are in the same subnet
  if (!validateGatewayAndIPAreInTheSameSubnet(temp_network_ip, temp_network_gateway, temp_network_subnet)) {
    return 4;
  }

  // Validate SSID and password length
  if (temp_ssid.equalsIgnoreCase("") || temp_password.equalsIgnoreCase("") || temp_ssid.equalsIgnoreCase("") || isExceedString(temp_password)) {
    return 5;
  }

  return 0;
}


bool isValidIPAddress(String ip) {
  // Split the string into octets
  int octets[4] = { 0 };
  int octetIndex = 0;
  String temp = "";

  for (unsigned int i = 0; i < ip.length(); i++) {
    if (ip[i] == '.') {
      if (octetIndex > 3) return false;  // More than 4 octets
      octets[octetIndex++] = temp.toInt();
      temp = "";
    } else {
      temp += ip[i];
    }
  }

  if (octetIndex != 3) return false;  // Less than 4 octets
  octets[octetIndex] = temp.toInt();  // Last octet

  // Check each octet is between 0 and 255
  for (int i = 0; i < 4; i++) {
    if (octets[i] < 0 || octets[i] > 255) return false;
  }

  return true;
}


uint32_t ipToUInt32(String ip) {
  uint32_t result = 0;
  int octetIndex = 0;
  String temp = "";

  for (unsigned int i = 0; i < ip.length(); i++) {
    if (ip[i] == '.') {
      result |= (temp.toInt() << (8 * (3 - octetIndex++)));
      temp = "";
    } else {
      temp += ip[i];
    }
  }
  result |= temp.toInt();  // Last octet

  return result;
}

// Function to check if the IP address and gateway are in the same subnet
bool validateGatewayAndIPAreInTheSameSubnet(String ip, String gateway, String subnet) {
  uint32_t ipAddr = ipToUInt32(ip);
  uint32_t gatewayAddr = ipToUInt32(gateway);
  uint32_t subnetMask = ipToUInt32(subnet);

  uint32_t networkIP = ipAddr & subnetMask;
  uint32_t networkGateway = gatewayAddr & subnetMask;

  return networkIP == networkGateway;
}


bool validateSubnet(String mask) {

  // Split the string into octets
  int octets[4] = { 0 };
  int octetIndex = 0;
  String temp = "";

  for (unsigned int i = 0; i < mask.length(); i++) {
    if (mask[i] == '.') {
      if (octetIndex > 3) return false;  // More than 4 octets
      octets[octetIndex++] = temp.toInt();
      temp = "";
    } else {
      temp += mask[i];
    }
  }

  if (octetIndex != 3) return false;  // Less than 4 octets
  octets[octetIndex] = temp.toInt();  // Last octet

  // Check each octet is between 0 and 255
  for (int i = 0; i < 4; i++) {
    if (octets[i] < 0 || octets[i] > 255) return false;
  }

  // Convert to a single 32-bit number
  uint32_t subnetMask = 0;
  for (int i = 0; i < 4; i++) {
    subnetMask = (subnetMask << 8) | octets[i];
  }

  // Check for contiguous 1s followed by contiguous 0s
  bool seenZero = false;
  for (int i = 31; i >= 0; i--) {
    if (subnetMask & (1 << i)) {
      if (seenZero) return false;  // 1 after 0 is invalid
    } else {
      seenZero = true;
    }
  }

  return true;
}


// Function to check if a string exceeds 32 characters
bool isExceedString(const String &str) {
  return str.length() > 32;
}


bool validateModbus(const String &modbus) {
  return isNumberString(modbus);
}


bool validatePin(const String &pin) {
  return isNumberString(pin);
}


bool validateStatus(const String &status) {
  return status.equalsIgnoreCase("on") || status.equalsIgnoreCase("off");
}


int validateNetwork(String str) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, str);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return 1;  // Not a valid JSON
  }
  String temp_ssid = doc["ssid"].as<String>();
  String temp_pass = doc["password"].as<String>();
  if (temp_ssid.isEmpty() || temp_pass.isEmpty() || temp_ssid.length() > 32 || temp_pass.length() < 8 || temp_pass.length() > 32) {
    return 2;  // Invalid format or missing required fields
  }
  return 0;
}


int validatePortConfig(String str) {
  // Check if the input is a valid JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, str);
  if (error) {
    return 1;  // Not a valid JSON format
  }

  // Iterate over the JSON object
  for (JsonPair kv : doc.as<JsonObject>()) {
    JsonObject port = kv.value().as<JsonObject>();

    // Check each key-value pair
    for (JsonPair innerKv : port) {
      const char *key = innerKv.key().c_str();
      JsonVariant value = innerKv.value();

      // Check if value is a string
      if (!value.is<const char *>()) {
        return 2;  // Every value must be in string format
      }

      const char *strValue = value.as<const char *>();

      // Check for null or empty values
      if (strValue == nullptr || strlen(strValue) == 0) {
        return 5;  // Every value must not be null or empty
      }

      // Specific checks for "status", "modbus", and "pin"
      if (strcmp(key, "status") == 0) {
        if (strcasecmp(strValue, "on") != 0 && strcasecmp(strValue, "off") != 0) {
          return 3;  // Status must be either "on" or "off"
        }
      } else if (strcmp(key, "modbus") == 0 || strcmp(key, "pin") == 0) {
        for (size_t i = 0; i < strlen(strValue); ++i) {
          if (!isdigit(strValue[i])) {
            return 4;  // Modbus and pin must represent an integer
          }
          if (String(key) == "modbus" && String(strValue[i]) == "0") {
            return 6;  // Modbus must represent an integer greater than 0
          }
        }
      }
    }
  }
  return 7;  // JSON is valid
}


void handlePostError(WebServer* server, const String message, const String dataLabel, const String dataInput) {
  StaticJsonDocument<200> doc;
  doc["status"] = "Failed";
  doc["message"] = message;
  doc["data"][dataLabel] = dataInput;
  String data_message;
  serializeJson(doc, data_message);
  server->send(400, "application/json", data_message);
}


void createDefaultConfigs() {
  Serial.println("Creating default configuration files...");
  
  // Create default port config
  if (!filemanager.checkFile(configPortpath.c_str())) {
    Serial.println("Creating default port configuration...");
    String defaultPortConfig = R"({
  "port1": {"sensor": "disable", "modbus": 1, "pin": 14},
  "port2": {"sensor": "disable", "modbus": 2, "pin": 22},
  "port3": {"sensor": "disable", "modbus": 3, "pin": 17},
  "port4": {"sensor": "disable", "modbus": 4, "pin": 35},
  "port5": {"sensor": "disable", "modbus": 5, "pin": 21},
  "port6": {"sensor": "disable", "modbus": 6, "pin": 27},
  "port7": {"sensor": "disable", "modbus": 7, "pin": 16},
  "port8": {"sensor": "servo", "modbus": 8, "pin": 13}
})";
    int writeResult = filemanager.writeFile(configPortpath.c_str(), defaultPortConfig.c_str());
    if (writeResult == 1) {
      Serial.println("✓ Default port configuration created successfully!");
    } else {
      Serial.println("✗ Failed to create default port configuration!");
    }
  }
  
  // Create default device config
  if (!filemanager.checkFile(configDevicepath.c_str())) {
    Serial.println("Creating default device configuration...");
    String defaultDeviceConfig = R"({
  "modbus_slave": {"id": 9, "state": "disable"},
  "hostname": "ExtenderEnvDevice",
  "name": "ExtenderEnvDevice",
  "sn": "123456789"
})";
    int writeResult = filemanager.writeFile(configDevicepath.c_str(), defaultDeviceConfig.c_str());
    if (writeResult == 1) {
      Serial.println("✓ Default device configuration created successfully!");
    } else {
      Serial.println("✗ Failed to create default device configuration!");
    }
  }
  
  Serial.println("Default configurations process completed!");
}

void readConfig() {
  Serial.println("Starting configuration read...");
  triggerTochangeSettingPort = true;
  
  // Create default configs if they don't exist
  createDefaultConfigs();
  delay(500);
  
  Serial.println("Reading Network STA Config...");
  readNetworkSTAConfig();
  delay(100);
  
  Serial.println("Reading ETH Config...");
  readETHConfig();
  delay(100);
  
  Serial.println("Reading Port Config...");
  readPortConfig();
  delay(100);
  
  Serial.println("Reading Device Config...");
  readDeviceConfig();
  delay(100);
  
  Serial.println("Configuration read completed successfully!");
}

void readETHConfig() {
  Serial.println("---------- Read Config Ethernet ----------");
  String eth_setting = filemanager.readFile(addressConfigEth.c_str());

  if (eth_setting.equals("")) {
    Serial.println("- Ethernet Config : N/A (using defaults)");
    Serial.println("Mode: " + String(dhcp_eth ? "DHCP" : "Static"));
    Serial.println("IP: " + ip_eth_conf.toString());
    Serial.println("Gateway: " + gateway_eth_conf.toString());
    Serial.println("Subnet: " + subnet_eth_conf.toString());
    Serial.println("DNS: " + dns1_eth_conf.toString());
    Serial.println("--------------------------------------\n");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, eth_setting);
  
  if (error) {
    Serial.println("- Ethernet Config : ERROR parsing JSON - " + String(error.c_str()));
    Serial.println("Using default configuration");
    Serial.println("--------------------------------------\n");
    return;
  }

  Serial.println("- Ethernet Config : " + eth_setting);
  
  if (doc.containsKey("mode")) {
    String mode = doc["mode"].as<String>();
    if (mode.equalsIgnoreCase("dhcp")) {
      dhcp_eth = true;
      Serial.println("Mode: DHCP");
    } else if (mode.equalsIgnoreCase("static")) {
      dhcp_eth = false;
      Serial.println("Mode: Static");
      
      // Read static IP configuration
      if (doc.containsKey("ip") && !doc["ip"].isNull()) {
        String ipStr = doc["ip"].as<String>();
        if (isValidIPAddress(ipStr)) {
          ip_eth_conf.fromString(ipStr);
          Serial.println("IP: " + ip_eth_conf.toString());
        }
      }
      
      if (doc.containsKey("gateway") && !doc["gateway"].isNull()) {
        String gatewayStr = doc["gateway"].as<String>();
        if (isValidIPAddress(gatewayStr)) {
          gateway_eth_conf.fromString(gatewayStr);
          Serial.println("Gateway: " + gateway_eth_conf.toString());
        }
      }
      
      if (doc.containsKey("subnet") && !doc["subnet"].isNull()) {
        String subnetStr = doc["subnet"].as<String>();
        if (isValidIPAddress(subnetStr)) {
          subnet_eth_conf.fromString(subnetStr);
          Serial.println("Subnet: " + subnet_eth_conf.toString());
        }
      }
      
      if (doc.containsKey("dns") && !doc["dns"].isNull()) {
        String dnsStr = doc["dns"].as<String>();
        if (isValidIPAddress(dnsStr)) {
          dns1_eth_conf.fromString(dnsStr);
          Serial.println("DNS: " + dns1_eth_conf.toString());
        }
      }
    }
  }

  trigger_to_change_eth_setup_ontrue = true;
  Serial.println("-----------------------------------------\n");
}


void readPortConfig() {
  Serial.println("---------- Read Config Port ----------");
  String port_config = filemanager.readFile(configPortpath.c_str());
  
  if (port_config.equals("")) {
    Serial.println("- Port Config : N/A (File not found or empty)");
    Serial.println("--------------------------------------\n");
    return;
  }
  
  Serial.println("- Raw Port Config : " + port_config);
  
  StaticJsonDocument<384> req;
  DeserializationError error = deserializeJson(req, port_config);
  
  if (error) {
    Serial.println("- Port Config : JSON Parse Error - " + String(error.c_str()));
    Serial.println("--------------------------------------\n");
    return;
  }
  
  if (req.isNull()) {
    Serial.println("- Port Config : Invalid JSON structure");
    Serial.println("--------------------------------------\n");
    return;
  }
  
  // Check if all required port configurations exist
  bool allPortsExist = true;
  for (int i = 1; i <= 8; i++) {
    String portKey = "port" + String(i);
    if (!req.containsKey(portKey)) {
      Serial.println("- Port Config : Missing " + portKey);
      allPortsExist = false;
    }
  }
  
  if (!allPortsExist) {
    Serial.println("- Port Config : Incomplete configuration");
    Serial.println("--------------------------------------\n");
    return;
  } else {
    String portSensors[8] = {
      req["port1"]["sensor"].as<String>(),
      req["port2"]["sensor"].as<String>(),
      req["port3"]["sensor"].as<String>(),
      req["port4"]["sensor"].as<String>(),
      req["port5"]["sensor"].as<String>(),
      req["port6"]["sensor"].as<String>(),
      req["port7"]["sensor"].as<String>(),
      req["port8"]["sensor"].as<String>()
    };

    int portModbusIds[8] = {
      req["port1"]["modbus"].as<int>(),
      req["port2"]["modbus"].as<int>(),
      req["port3"]["modbus"].as<int>(),
      req["port4"]["modbus"].as<int>(),
      req["port5"]["modbus"].as<int>(),
      req["port6"]["modbus"].as<int>(),
      req["port7"]["modbus"].as<int>(),
      req["port8"]["modbus"].as<int>()
    };

    int portPins[8] = {
      req["port1"]["pin"].as<int>(),
      req["port2"]["pin"].as<int>(),
      req["port3"]["pin"].as<int>(),
      req["port4"]["pin"].as<int>(),
      req["port5"]["pin"].as<int>(),
      req["port6"]["pin"].as<int>(),
      req["port7"]["pin"].as<int>(),
      req["port8"]["pin"].as<int>()
    };

    PORT_CONFIGURATED.setConfig(portSensors, portModbusIds, portPins);

    for (int i = 0; i < 8; i++) {
      Serial.print("Port ");
      Serial.print(i + 1);
      Serial.println(" ");
      Serial.print("- Status : ");
      Serial.println(req["port" + String(i + 1)]["sensor"].as<String>());
      Serial.print("- Modbus Id : ");
      Serial.println(req["port" + String(i + 1)]["modbus"].as<String>());
      Serial.print("- Pin : ");
      Serial.println(req["port" + String(i + 1)]["pin"].as<String>());
    }
    Serial.println("--------------------------------------\n");
  }
}

void readDeviceConfig() {
  Serial.println("---------- Read Config Device ----------");
  String device_config = filemanager.readFile(configDevicepath.c_str());
  if (device_config.equals("")) {
    Serial.println("- Device Config : N/A");
    Serial.println("--------------------------------------\n");
    return;
  } else {
    StaticJsonDocument<384> req;
    DeserializationError error = deserializeJson(req, device_config);
    if (error) {
      Serial.println("- Port Config : Error File System " + String(error.c_str()));
      return;
    }

    DEVICE_CONFIGURATED.setConfig(req["modbus_slave"]["id"].as<int>(),
                                  req["modbus_slave"]["state"].as<String>(),
                                  req["hostname"].as<String>(),
                                  req["name"].as<String>(),
                                  req["sn"].as<String>());

    Serial.print("Modbus Slave ID : ");
    Serial.println(req["modbus_slave"]["id"].as<int>());
    Serial.print("Modbus Slave State : ");
    Serial.println(req["modbus_slave"]["state"].as<String>());
    Serial.print("Device Hostname : ");
    Serial.println(req["hostname"].as<String>());
    Serial.print("Device Name : ");
    Serial.println(req["name"].as<String>());
    Serial.print("Device SN : ");
    Serial.println(req["sn"].as<String>());
    Serial.println("--------------------------------------\n");
  }
}

void readNetworkSTAConfig() {
  Serial.println("---------- Read Config Wireless ----------");
  String wireless_setting = filemanager.readFile(networkSTApath.c_str());
  
  if (wireless_setting.equals("")) {
    Serial.println("- Wireless Config : N/A (File not found or empty)");
    Serial.println("--------------------------------------\n");
    return;
  }
  
  Serial.println("- Raw Wireless Config : " + wireless_setting);
  
  StaticJsonDocument<384> doc;
  DeserializationError error = deserializeJson(doc, wireless_setting);
  
  if (error) {
    Serial.println("- Wireless Config : JSON Parse Error - " + String(error.c_str()));
    Serial.println("--------------------------------------\n");
    return;
  }
  
  if (doc.isNull()) {
    Serial.println("- Wireless Config : Invalid JSON structure");
    Serial.println("--------------------------------------\n");
    return;
  } else {
    Serial.println("- Wireless Config : " + wireless_setting);
    if (!doc["ssid"].as<String>().equalsIgnoreCase("") && doc.containsKey("ssid")) {
      if (doc["mode"].as<String>().equalsIgnoreCase("dhcp")) {
        dhcp_wireless = true;

      } else {
        ip_station_conf.fromString(doc["ip"].as<String>());
        subnet_station_conf.fromString(doc["subnet"].as<String>());
        gateway_station_conf.fromString(doc["gateway"].as<String>());
        dns1_station_conf.fromString(doc["dns"].as<String>());
        dhcp_wireless = false;
      }

      ssid_station = doc["ssid"].as<String>();
      password_station = doc["password"].as<String>();
      // ssid = ??? if you want to DebugHandl
      // password = ?? put here too
      trigger_wireless = 1;
    }

    Serial.println("------------------------------------------\n");
  }
}


bool writeNetworkSTAConfig(String ssid, String pass) {
  Serial.println("fn begin writeNetworkSTAConfig");
  String output;
  StaticJsonDocument<256> doc;
  doc["ssid"] = ssid;
  doc["password"] = pass;
  serializeJson(doc, output);
  Serial.println("DEBUG :" + output);
  int writeState = filemanager.writeFile(networkSTApath.c_str(), output.c_str());
  if (writeState == 1) {
    return true;
  } else {
    return false;
  }
}
void readEthConfig() {
  String dataEthNetworktemp = filemanager.readFile(addressConfigEth.c_str());
  StaticJsonDocument<384> doc;
  deserializeJson(doc, dataEthNetworktemp);
  Serial.println("flash read eth:" + dataEthNetworktemp);
  dhcp_eth = true;

  if (doc["mode"].as<String>().equalsIgnoreCase("static")) {
    dhcp_eth = false;
    ip_eth_conf.fromString(doc["ip"].as<String>());
    subnet_eth_conf.fromString(doc["subnet"].as<String>());
    gateway_eth_conf.fromString(doc["gateway"].as<String>());
    dns1_eth_conf.fromString(doc["dns1"].as<String>());
    dns2_eth_conf.fromString(doc["dns2"].as<String>());
  }
}

// Wrapper agar seragam dengan deklarasi di main_function.ino
void readNetworkETHConfig() {
  readEthConfig();
}

void fromStringMacAddress(const String &macStr, uint8_t *macArray) {
  int mac[6];  // Temporary array to hold the MAC address as integers
  sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

  // Copy the integers into the uint8_t array
  for (int i = 0; i < 6; i++) {
    macArray[i] = (uint8_t)mac[i];
  }
}



bool IPisequal(IPAddress ip1, IPAddress ip2) {
  for (int i = 0; i < 4; i++) {
    if (ip1[i] != ip2[i]) {
      return false;  // If any byte is not equal, return false
    }
  }
  return true;  // All bytes are equal, return true
}


int validateInputforEthernet(String temp_network_ip, String temp_network_subnet,
                             String temp_network_gateway, String temp_network_dns) {
  // Validate if not null
  if (temp_network_ip.equalsIgnoreCase("") || temp_network_subnet.equalsIgnoreCase("") || temp_network_gateway.equalsIgnoreCase("")) {
    return 1;
  }

  // Validate the IPs are correct
  if (!isValidIPAddress(temp_network_ip) || !isValidIPAddress(temp_network_subnet) || !isValidIPAddress(temp_network_gateway)) {
    return 2;
  }

  // Validate subnet validity
  if (!validateSubnet(temp_network_subnet)) {
    return 3;
  }

  // Validate if gateway and IP are in the same subnet
  if (!validateGatewayAndIPAreInTheSameSubnet(temp_network_ip, temp_network_gateway, temp_network_subnet)) {
    return 4;
  }


  return 0;
}

