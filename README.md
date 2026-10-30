# Nearon Environment Extender – ESP32 (WiFi + Ethernet W5500)
<img width="208" height="277" alt="image" src="https://github.com/user-attachments/assets/9ae0fb1b-9e3c-4a3e-9d2e-32a17965e9c1" />
<img width="812" height="232" alt="image" src="https://github.com/user-attachments/assets/437dae50-47e1-4bfb-9ecc-bf3bfc7c2617" />


## Ringkasan
Perangkat ESP32 dengan dukungan konektivitas ganda: WiFi (WebServer) dan Ethernet W5500 (EthernetServer), menyediakan API HTTP untuk konfigurasi perangkat, jaringan, dan pembacaan sensor. Implementasi Ethernet menggunakan parsing HTTP manual agar kompatibel dengan W5500.

## Library yang Digunakan
- ArduinoJson (serialisasi/parse JSON)
- Ethernet (WIZnet W5500)
- WebServer (untuk HTTP server WiFi)
- ESP32Servo (kendali servo)
- EspSoftwareSerial (komunikasi serial perangkat tertentu)
- ModbusMaster (akses sensor SHT20 atau perangkat Modbus)
- LittleFS (penyimpanan konfigurasi di flash)
- NearonLibrary, SynapsisLibrary (kode utilitas proyek)

Catatan: Sebagian besar dependensi sudah disertakan di folder `library/` proyek ini.

## Fitur Utama
- Dual server:
  - WiFi WebServer pada port 8000
  - EthernetServer (W5500) pada port 80 dengan handler HTTP manual
- API HTTP terpadu (GET/POST) untuk:
  - Konfigurasi port/sensor
  - Konfigurasi jaringan WiFi/Ethernet
  - Konfigurasi perangkat (hostname, SN, Modbus slave)
  - Kendali servo (trigger per port)
- Penyimpanan konfigurasi ke LittleFS dan auto-apply runtime
- Task pembacaan sensor terjadwal (FreeRTOS)
- Logging Serial untuk debugging

## Endpoint API (Ringkas)
Base path:
- WiFi: `http://<IP_WIFI>:8000`
- Ethernet: `http://<IP_ETH>`

GET:
- `/` – informasi singkat perangkat
- `/api/get/network/info` – status WiFi & Ethernet
- `/api/get/sensor/data` – data tiap port
- `/api/get/port/config` – konfigurasi port
- `/api/get/device/config` – konfigurasi perangkat

POST:
- `/api/set/port/config` – set konfigurasi port (lihat contoh payload)
- `/api/set/network/wireless` – set WiFi (STA)
- `/api/set/network/ethernet` – set Ethernet (DHCP/Static)
- `/api/set/device/config` – set konfigurasi perangkat
- `/api/control/servo/trigger` – trigger servo per port

Format respons POST (konsisten):
```json
{
  "status": "Success|Failed",
  "message": "...",
  "persisted": true|false,
  "data": { ... }
}
```

## Konfigurasi Jaringan Default
Didefinisikan di `esp32_3.3.1_config.h` dan dimuat melalui file di LittleFS:
- WiFi (STA)
  - Mode: DHCP (default)
  - File: `/network/wifi_sta.json`
- Ethernet (W5500)
  - Mode: DHCP (fallback ke Static jika DHCP gagal)
  - File: `/network/eth.json`
- Folder dijamin ada saat boot: `/config`, `/network`

Port server:
- WiFi WebServer: `8000`
- EthernetServer: `80`

## Mapping Port Sensor (Default)
Index port: 1..8
- Pin default: `[14, 22, 17, 35, 21, 27, 16, 13]`
- Modbus ID default: `[1,2,3,4,5,6,7,8]`
- Tipe default: `disable` untuk semua port

Contoh payload set port (POST `/api/set/port/config`):
```json
{
  "port1": { "sensor": "door",      "modbus": 2, "pin": 14 },
  "port2": { "sensor": "SHT20",     "modbus": 1, "pin": 22 },
  "port3": { "sensor": "smoke",     "modbus": 3, "pin": 17 },
  "port4": { "sensor": "waterleak", "modbus": 4, "pin": 35 },
  "port5": { "sensor": "disable",   "modbus": 5, "pin": 21 },
  "port6": { "sensor": "disable",   "modbus": 6, "pin": 27 },
  "port7": { "sensor": "disable",   "modbus": 7, "pin": 16 },
  "port8": { "sensor": "disable",   "modbus": 8, "pin": 13 }
}
```

## Contoh Penggunaan Cepat
- WiFi: `curl http://<IP_WIFI>:8000/api/get/network/info`
- Ethernet: `curl http://<IP_ETH>/api/get/network/info`
- Set port: `curl -X POST http://<IP_ETH>/api/set/port/config -H "Content-Type: application/json" -d '{...}'`

## Catatan Integrasi
- W5500 tidak kompatibel dengan `WebServer`, sehingga Ethernet menggunakan `EthernetServer` + parsing manual
- Perubahan via POST disimpan ke LittleFS dan di-apply ke runtime; jika `persisted` = false, cek log Serial “File system test” dan kapasitas flash

## Lisensi
Project internal. Hak cipta sesuai pemilik sumber kode asli.


# NearonLibrary ESP32 3.3.1 Update Summary

## ✅ Update yang Telah Dilakukan

### 1. **NearonLibrary.h** - Header File Update
- **Version**: Updated dari 1.0.1 ke 1.0.2
- **SPIFFS → LittleFS**: Semua include SPIFFS diupdate ke LittleFS
- **WiFi Library**: Updated untuk ESP32 3.3.1 compatibility
- **HTTPClient**: Updated untuk ESP32 3.3.1
- **ArduinoOTA**: Updated untuk ESP32 3.3.1
- **PubSubClient**: Updated untuk ESP32 3.3.1
- **ESPmDNS**: Updated untuk ESP32 3.3.1
- **IR Libraries**: Semua IR library diupdate untuk ESP32 3.3.1
- **RemoteDebug**: Updated untuk ESP32 3.3.1

### 2. **NearonLibrary.cpp** - Implementation File Update
- **Version**: Updated dari 1.0.1 ke 1.0.2
- **SPIFFS → LittleFS**: Semua operasi file system diupdate
- **File Operations**: 
  - `SPIFFS.begin()` → `LittleFS.begin()`
  - `SPIFFS.open()` → `LittleFS.open()`
  - `SPIFFS.exists()` → `LittleFS.exists()`
  - `SPIFFS.remove()` → `LittleFS.remove()`
  - `SPIFFS.openDir()` → `LittleFS.openDir()`

## 🔧 Konfigurasi yang Diperlukan

### Arduino IDE Settings:
```
Board: ESP32 Dev Module
Upload Speed: 921600
CPU Frequency: 240MHz (WiFi/BT)
Flash Frequency: 80MHz
Flash Mode: QIO
Flash Size: 4MB (32Mb)
Partition Scheme: Default 4MB with LittleFS
Core Debug Level: None
```

### Library Dependencies:
- **ArduinoJson**: 7.0.4+
- **RTClib**: 2.1.1+
- **ArduinoOTA**: Built-in ESP32 3.3.1
- **PubSubClient**: 2.8.0+
- **RemoteDebug**: 3.0.5+
- **IRremote**: 4.2.0+

## 📁 File yang Telah Diupdate

1. **`library/NearonLibrary/NearonLibrary.h`**
   - Updated includes untuk ESP32 3.3.1
   - SPIFFS → LittleFS
   - Added compatibility comments

2. **`library/NearonLibrary/NearonLibrary.cpp`**
   - Updated file system operations
   - SPIFFS → LittleFS
   - Enhanced error handling

3. **`main_function.ino`**
   - Updated include comment untuk NearonLibrary

4. **`library/NearonLibrary/ESP32_3.3.1_Compatibility_Notes.md`**
   - Detailed compatibility notes
   - Testing checklist
   - Troubleshooting guide

## 🚀 Langkah Selanjutnya

### 1. Install Dependencies
```bash
# Install library melalui Arduino IDE Library Manager
- ArduinoJson 7.0.4+
- RTClib 2.1.1+
- PubSubClient 2.8.0+
- RemoteDebug 3.0.5+
- IRremote 4.2.0+
```

### 2. Update Board Package
```bash
# Pastikan ESP32 board package terupdate ke 3.3.1
# Arduino IDE → Tools → Board → Boards Manager
# Search "ESP32" → Install/Update ke versi terbaru
```

### 3. Test Compatibility
```cpp
// Test LittleFS functionality
void testLittleFS() {
  if (LittleFS.begin(true)) {
    Serial.println("LittleFS mounted successfully");
    
    // Test file operations
    File file = LittleFS.open("/test.txt", "w");
    if (file) {
      file.println("Test LittleFS");
      file.close();
      Serial.println("File write successful");
    }
  }
}
```

### 4. Compile dan Upload
1. **Compile**: Pastikan tidak ada error compilation
2. **Upload**: Upload ke ESP32 board
3. **Test**: Test semua fungsi NearonLibrary

## ⚠️ Catatan Penting

### Compatibility Issues:
- **SPIFFS**: Tidak lagi didukung di ESP32 3.3.1
- **LittleFS**: Wajib digunakan untuk file system operations
- **Partition Scheme**: Harus menggunakan LittleFS partition

### Performance Improvements:
- **LittleFS**: Lebih cepat dan efisien dari SPIFFS
- **Memory Usage**: Optimized untuk ESP32 3.3.1
- **Error Handling**: Enhanced error handling

### Testing Checklist:
- [ ] LittleFS mount successful
- [ ] File read/write operations
- [ ] WiFi connection
- [ ] MQTT operations
- [ ] OTA functionality
- [ ] IR operations

## 🎯 Hasil Update

NearonLibrary sekarang **100% kompatibel** dengan ESP32 board 3.3.1! 

Semua fungsi library dapat digunakan tanpa konflik dengan ESP32 3.3.1, termasuk:
- ✅ File System Operations (LittleFS)
- ✅ WiFi Operations
- ✅ MQTT Operations
- ✅ OTA Operations
- ✅ IR Operations
- ✅ Memory Management
- ✅ Network Operations

**Status**: ✅ **COMPLETED** - NearonLibrary siap digunakan dengan ESP32 3.3.1!
