# Pool Pump Controller using LilyGO T-Relay ESP32

[![ESP32 CI](https://github.com/andersCTO/poolPumpControl/actions/workflows/esp32-ci.yml/badge.svg)](https://github.com/andersCTO/poolPumpControl/actions/workflows/esp32-ci.yml)
[![Quality Checks](https://github.com/andersCTO/poolPumpControl/actions/workflows/quality-checks.yml/badge.svg)](https://github.com/andersCTO/poolPumpControl/actions/workflows/quality-checks.yml)
[![Test Suite](https://github.com/andersCTO/poolPumpControl/actions/workflows/test-suite.yml/badge.svg)](https://github.com/andersCTO/poolPumpControl/actions/workflows/test-suite.yml)

This project automates the control of a pool circulation pump and heater to minimize electricity costs while maintaining proper water quality and temperature. It uses a [LilyGO T-Relay ESP32 module](https://github.com/Xinyuan-LilyGO/LilyGo-T-Relay) and connects to an **AquaForte VARIO+ II** (model Vario+ 1100) frequency inverter via digital output.

## Project Goals

1. **Automated pool water circulation** based on electricity prices.
2. **Optimized pump speed control** via RS485 or digital relay control.
3. **Future expansion** to include heater control and external sensors (e.g., temperature).
4. **WiFi-connected** and fetches day-ahead electricity spot prices automatically.

---

## Hardware

- 🧠 **Microcontroller**: LilyGO T-Relay ESP32
- 🔌 **Frequency Inverter**: AquaForte VARIO+ II (Vario+ 1100)
- 🌀 **Pump**: Single-phase pump with permanent split capacitor motor (PSC)
- 🔌 **Power**: 220-240V AC

---

## Platform

- **Firmware**: [Espressif IoT Development Framework (ESP-IDF)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)
- **Development Environment**: Visual Studio Code with Espressif plugin
- **Language**: C/C++

---

## Features

- ✅ Automatically connects to WiFi
- 📈 Fetches 24h electricity price data from API
- ⚙️ Controls pump speed (1400, 2000, 2900 RPM)
- 🌡️ Can integrate outdoor temperature for smarter logic
- ⏱️ Supports timed operation modes: Day, Night, Backwash
- 💾 Remembers settings across reboots
- 📊 Displays current RPM and power usage
- 📱 **Bluetooth mobile configuration interface** (WiFi setup via BLE)

---

## Mobile Configuration via Bluetooth

The system supports configuration through a mobile app using Bluetooth Low Energy (BLE). This allows for easy setup without needing to hardcode WiFi credentials.

### 🔵 Bluetooth Features

- **Initial Setup**: Configure WiFi SSID and password via mobile app
- **Future Capabilities**: 
  - Pump settings (speed, runtime schedules)
  - Price thresholds configuration
  - System status and diagnostics
  - Schedule management

### 📱 How to Use

1. **Power on the ESP32** - It will start advertising as "PoolPump-ESP32"
2. **Open your mobile app** and scan for BLE devices
3. **Connect to "PoolPump-ESP32"**
4. **Send WiFi credentials** through the BLE interface
5. **ESP32 connects to WiFi** and starts normal operation

### 🔧 BLE Service Structure

The Bluetooth interface uses a custom GATT service (UUID: 0x00FF) with the following characteristics:

| Characteristic | UUID | Type | Description |
|----------------|------|------|-------------|
| WiFi SSID | 0xFF01 | Write | Network name to connect to |
| WiFi Password | 0xFF02 | Write | Network password |
| Pump Settings | 0xFF03 | Read/Write | Pump speed and schedule settings |
| Price Settings | 0xFF04 | Read/Write | Electricity price thresholds |
| System Info | 0xFF05 | Read/Notify | Current system status |
| Notifications | 0xFF06 | Notify | System messages and alerts |

### 📚 Documentation

Detailed documentation for the Bluetooth interface is available in:
- [`components/bluetooth_config/README.md`](components/bluetooth_config/README.md) - Complete API and usage guide (Swedish)
- [`components/bluetooth_config/bluetooth_config.h`](components/bluetooth_config/bluetooth_config.h) - API reference

### 🔐 Security

- BLE communication is encrypted
- WiFi credentials are transmitted securely
- Only one device can connect at a time
- Automatic timeout after inactivity

---

## Inverter Configuration

The AquaForte Vario+ II supports:
- **Speed ranges**: 1200–2900 RPM
- **3 user-defined modes**:
  - Night: 1400 RPM
  - Day: 2000 RPM
  - Backwash: 2900 RPM
- **Self-priming**: 1 min at full speed on startup
- **Timer support**: Up to 4 scheduled intervals per day
- **External digital control** via COM + DI2/DI3/DI4

📘 Refer to the [manual](docs/RELAY_ESP32.md) and the [Vario+ 1100 documentation](rb344-vario-manual.pdf) for wiring and configuration.

---

## Installation

1. Clone the repository.
2. Set up your development environment with ESP-IDF and Visual Studio Code.
3. Configure WiFi credentials and API endpoint for electricity prices.
4. Connect the relay output to the digital input pins of the Vario+ II.
5. Flash the firmware to your ESP32 T-Relay board.

---

## Project Structure

The firmware follows a standard ESP-IDF layout that keeps hardware access, networking, and business logic separated into reusable components:

```
poolPumpControl/
├── CMakeLists.txt           # Top-level project definition
├── Kconfig.projbuild        # Configuration options exposed in menuconfig
├── main/
│   ├── CMakeLists.txt
│   └── main.c               # Entry point that starts the application core
├── components/
│   ├── app_core/            # High-level orchestration and state machine
│   ├── bluetooth_config/    # BLE interface for mobile app configuration
│   ├── networking/          # WiFi provisioning and connectivity helpers
│   ├── price_client/        # Electricity price fetching logic
│   ├── pump_driver/         # Relay and inverter control primitives
│   ├── scheduler/           # Price-aware scheduling routines
│   ├── sensors/             # Temperature and flow sensor interfaces
│   └── storage/             # Persistent configuration helpers
├── docs/
│   └── RELAY_ESP32.md       # Hardware wiring notes (placeholder)
└── .gitignore
```

Each component exposes public headers under `include/pool_pump/` so that functionality can be shared without tight coupling. This structure scales with additional features such as heater control or OTA updates by adding dedicated components.

---

## CI/CD Pipeline

This project uses GitHub Actions for continuous integration and deployment:

### Workflows

- **ESP32 CI** (`esp32-ci.yml`): Builds the project and verifies compilation
- **Quality Checks** (`quality-checks.yml`): Runs static analysis, formatting checks, and code quality validation
- **Test Suite** (`test-suite.yml`): Validates test compilation and structure (tests run on hardware)
- **Release** (`release.yml`): Creates automated releases with firmware binaries

### Quality Assurance

- **Code Formatting**: Enforced via clang-format with LLVM style
- **Static Analysis**: cppcheck for common C/C++ issues
- **Build Verification**: Ensures all components compile successfully
- **Test Validation**: Verifies test framework integrity

### Building Locally

```bash
# Setup ESP-IDF environment
source ~/esp/esp-idf/export.sh

# Build the project
idf.py build

# Flash to ESP32
idf.py flash

# Monitor serial output
idf.py monitor
```

### Running Tests

Tests are designed to run on ESP32 hardware. After flashing:

```bash
idf.py monitor
```

Test results will be displayed in the serial console.

---

## Mobile App Development

To create a mobile app for controlling the pool pump via Bluetooth:

### React Native Example

```javascript
import BleManager from 'react-native-ble-manager';

// Scan for devices
BleManager.scan([], 5, true).then(() => {
  console.log('Scanning for PoolPump-ESP32...');
});

// Connect and send WiFi credentials
BleManager.connect(peripheralId).then(() => {
  // Send WiFi SSID
  const ssidData = Buffer.from("MyWiFiNetwork");
  BleManager.write(peripheralId, '00FF', 'FF01', ssidData);
  
  // Send WiFi Password
  const passData = Buffer.from("MyPassword123");
  BleManager.write(peripheralId, '00FF', 'FF02', passData);
});
```

### Flutter/Dart Example

```dart
import 'package:flutter_blue/flutter_blue.dart';

// Find and connect to device
FlutterBlue flutterBlue = FlutterBlue.instance;
flutterBlue.scan().listen((scanResult) {
  if (scanResult.device.name == 'PoolPump-ESP32') {
    scanResult.device.connect();
  }
});

// Write WiFi credentials
await characteristic.write(utf8.encode('MyWiFiNetwork'));
```

---

## Future Improvements

- 🔥 Heater control via second relay
- 📡 OTA updates
- 🌤️ Integration of weather forecasts
- 🧠 Smart scheduling based on learning algorithms
- 📱 Complete mobile app for iOS/Android
- 🔔 Push notifications for system alerts
- 📊 Historical data and analytics

---

## Safety & Disclaimer

Ensure your pump motor is compatible (PSC motor only). Follow all electrical safety standards. Refer to the inverter manual for wiring and setup instructions:contentReference[oaicite:0]{index=0}.

---

## License

MIT License
