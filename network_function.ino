void WiFiBegin() {
  Serial.println("--- Setup Wireless ---");
  bool scan = Connections.scanWifi(ssid_station.c_str());
  if (scan == false) {
    dual_wifi = 1;
    Serial.println("AP Mode - Cannot Found SSID " + String(ssid_station));
    dataNetwork network = Connections.setAP(ssid_ap.c_str(), password_ap.c_str());
    ip_ap = network.ipaccesspoint;
    mac_ap = network.macaccesspoint;
    Serial.println("Generated Access Point " + String(ssid_ap.c_str()) + " with Password " + String(password_ap.c_str()));
    Serial.println("MAC Address AP : " + String(mac_ap));
    Serial.println("IP Address AP : " + String(ip_ap));

  } else {
    //    WiFi.disconnect();
    //    WiFi.softAPdisconnect();
    dual_wifi = 2;
    if (dhcp_wireless) {
      Serial.println("Dual Mode WiFi - DHCP");
      dataNetwork network = Connections.dualwifi(ssid_station.c_str(), password_station.c_str(), ssid_ap.c_str(), password_ap.c_str(), true);
      ip_ap = network.ipaccesspoint;
      ip_station = network.ipstation;
      mac_station = network.macstation;
      mac_ap = network.macaccesspoint;

    } else {

      dataNetwork network = Connections.dualwifi(ssid_station.c_str(), password_station.c_str(), ssid_ap.c_str(), password_ap.c_str(),
                            ip_station_conf, gateway_station_conf, subnet_station_conf, dns1_station_conf, dns2_station_conf);
      ip_ap = network.ipaccesspoint;
      ip_station = network.ipstation;
      mac_station = network.macstation;
      mac_ap = network.macaccesspoint;
    }
    Serial.println("Generated Access Point " + String(ssid_ap.c_str()) + " with Password " + String(password_ap.c_str()));
    Serial.println("MAC Address AP : " + String(mac_ap));
    Serial.println("IP Address AP : " + String(ip_ap));
    Serial.println("Connected to " + String(ssid_station.c_str()) + " with Password " + String(password_station.c_str()));
    Serial.println("MAC Address Station : " + String(mac_station));
    Serial.println("IP Address Station : " + String(ip_station));
  }
  WiFiState();
  trigger_wireless = 0;
  Serial.println("---------------------\n");
  delay(1000);
}



void WiFiState() {
  statusNetwork network = Connections.statusWifi();
  wifi_status = network.status;
  wifi_rssi = String(network.rssi);
}

void WiFiHandle() {
  if (dual_wifi == 1 && !ssid_station.equalsIgnoreCase("")) {

    Serial.println("connecting attemp " + String(couter_reset_network_if_not_connected_10_times));
    couter_reset_network_if_not_connected_10_times++;

    if (couter_reset_network_if_not_connected_10_times == 10) {
      Serial.println("attemp reach " + String(couter_reset_network_if_not_connected_10_times) + " try to reset connection ");
      ssid_station = "";
      password_station = "";
      dhcp_wireless = true;
      StaticJsonDocument<384> doc;
      doc["ssid"] = ssid_station;
      doc["password"] = password_station;
      doc["mode"] = "dhcp";
      doc["ip"] = "";
      doc["gateway"] = "";
      doc["subnet"] = "";
      doc["dns"] = "";
      String output;
      serializeJson(doc, output);
      if (!filemanager.writeFile(networkSTApath.c_str(), output.c_str())) {
        Serial.println("failed to write data network config");
      }

      couter_reset_network_if_not_connected_10_times = 1;
    }
    WiFiBegin();
  }
}


void ethBegin() {
  Serial.println("--- Setup Ethernet ---");
  Serial.println("CS Pin: " + String(csPinorSSPin));
  Serial.println("MAC Address: " + macETH);
  
  // Initialize ethernet with your proven configuration
  Ethernet.init(csPinorSSPin);
  
  // Convert MAC address string to byte array
  uint8_t mac[6];
  fromStringMacAddress(macETH, mac);
  
  Serial.println("Begin Ethernet Device");
  
  if (dhcp_eth) {
    Serial.println("Mode : DHCP");
    if (Ethernet.begin(mac)) { // Dynamic IP setup
      Serial.println("DHCP OK!");
    } else {
      Serial.println("Failed to configure Ethernet using DHCP");
      // Check for Ethernet hardware present
      if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield was not found. Sorry, can't run without hardware. :(");
        trigger_to_change_eth_setup_ontrue = true;
        return;
      }
      if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet cable is not connected.");
      }
      // Fallback to static IP
      Serial.println("Trying static IP as fallback...");
      dhcp_eth = false;
      ip_eth_conf = IPAddress(192, 168, 1, 28);
      gateway_eth_conf = IPAddress(192, 168, 1, 1);
      subnet_eth_conf = IPAddress(255, 255, 255, 0);
      dns1_eth_conf = IPAddress(192, 168, 1, 1);
      Ethernet.begin(mac, ip_eth_conf, dns1_eth_conf, gateway_eth_conf, subnet_eth_conf);
      Serial.println("STATIC OK!");
    }
  } else {
    Serial.println("Mode : Static");
    Serial.println("Static IP Configuration:");
    Serial.println("  IP: " + ip_eth_conf.toString());
    Serial.println("  Gateway: " + gateway_eth_conf.toString());
    Serial.println("  Subnet: " + subnet_eth_conf.toString());
    Serial.println("  DNS: " + dns1_eth_conf.toString());
    
    Ethernet.begin(mac, ip_eth_conf, dns1_eth_conf, gateway_eth_conf, subnet_eth_conf);
    Serial.println("STATIC OK!");
  }
  
  delay(1000);
  
  // Get actual configuration
  ip_eth_conf = Ethernet.localIP();
  subnet_eth_conf = Ethernet.subnetMask();
  gateway_eth_conf = Ethernet.gatewayIP();
  dns1_eth_conf = Ethernet.dnsServerIP();
  
  Serial.println("=== Ethernet Configuration ===");
  Serial.print("Local IP : ");
  Serial.println(ip_eth_conf);
  Serial.print("Subnet Mask : ");
  Serial.println(subnet_eth_conf);
  Serial.print("Gateway IP : ");
  Serial.println(gateway_eth_conf);
  Serial.print("DNS Server : ");
  Serial.println(dns1_eth_conf);
  Serial.println("Ethernet Successfully Initialized");
  
  // Test internet connectivity
  testInternetConnectivity();
  
  Serial.println("----------------------\n");
  trigger_to_change_eth_setup_ontrue = false;
}

void testInternetConnectivity() {
  Serial.println("Testing internet connectivity...");
  
  // Test internet connectivity with HTTP request
  EthernetClient client;
  char server[] = "httpbin.org";
  
  if (client.connect(server, 80)) {
    Serial.println("Connected to httpbin.org!");
    // Make a HTTP request:
    client.println("GET /get HTTP/1.1");
    client.println("Host: httpbin.org");
    client.println("Connection: close");
    client.println();
    
    // Wait for response
    unsigned long timeout = millis() + 5000;
    bool responseReceived = false;
    while (client.connected() && millis() < timeout) {
      if (client.available()) {
        String response = client.readString();
        if (response.indexOf("origin") > 0) {
          Serial.println("Internet test: SUCCESS - Got response from httpbin.org");
          responseReceived = true;
        }
        break;
      }
    }
    
    if (!responseReceived) {
      Serial.println("Internet test: TIMEOUT - No response received");
    }
    
    client.stop();
  } else {
    Serial.println("Internet connectivity: FAILED - Could not connect to httpbin.org");
    Serial.println("Check gateway and DNS settings");
  }
}
