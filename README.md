# Pool Pump Controller using LilyGO T-Relay ESP32

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

## Future Improvements

- 🔥 Heater control via second relay
- 📡 OTA updates
- 🌤️ Integration of weather forecasts
- 🧠 Smart scheduling based on learning algorithms

---

## Safety & Disclaimer

Ensure your pump motor is compatible (PSC motor only). Follow all electrical safety standards. Refer to the inverter manual for wiring and setup instructions:contentReference[oaicite:0]{index=0}.

---

## License

MIT License
