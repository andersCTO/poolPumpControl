# Pool Pump Controller

[![ESP32 CI](https://github.com/andersCTO/poolPumpControl/actions/workflows/esp32-ci.yml/badge.svg)](https://github.com/andersCTO/poolPumpControl/actions/workflows/esp32-ci.yml)
[![Quality Checks](https://github.com/andersCTO/poolPumpControl/actions/workflows/quality-checks.yml/badge.svg)](https://github.com/andersCTO/poolPumpControl/actions/workflows/quality-checks.yml)
[![Host Tests](https://github.com/andersCTO/poolPumpControl/actions/workflows/host-tests.yml/badge.svg)](https://github.com/andersCTO/poolPumpControl/actions/workflows/host-tests.yml)
[![Test Suite](https://github.com/andersCTO/poolPumpControl/actions/workflows/test-suite.yml/badge.svg)](https://github.com/andersCTO/poolPumpControl/actions/workflows/test-suite.yml)

ESP32 pool pump controller firmware that minimizes electricity costs by scheduling pump operation during the cheapest Nordpool spot price hours. Runs on a [LilyGO T-Relay](https://github.com/Xinyuan-LilyGO/LilyGo-T-Relay) board controlling an AquaForte VARIO+ II inverter via digital relay outputs.

## Features

- Fetches 24h Nordpool electricity spot prices and schedules pump operation in the cheapest hours
- Price-aware optimizer selects both *when* and *at what speed* to run (96 x 15-min slots/day)
- Three speed modes via relay-controlled inverter digital inputs: Low (1200 RPM), Medium (2400 RPM), High (2900 RPM)
- Web dashboard with real-time status, schedule visualization, and price display (`GET /`)
- JSON status API (`GET /api/status`)
- Web-based WiFi reconfiguration (`GET /wifi`, `POST /api/wifi`) with NVS persistence
- BLE GATT provisioning for initial WiFi setup via mobile app
- NVS-first credential lookup on boot (NVS > Kconfig fallback)
- Persistent configuration across reboots via NVS

## Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | LilyGO T-Relay ESP32 (4MB flash) |
| Inverter | AquaForte VARIO+ II (Vario+ 1100) |
| Pump | Single-phase PSC motor |
| Power | 220-240V AC |

### Relay-to-Inverter Mapping

Per the RB344 Vario manual (Section 5.4), each relay activates a fixed inverter speed:

| Relay | GPIO | Inverter DI | Speed | Mode |
|-------|------|-------------|-------|------|
| Relay 1 | 21 | DI2 → COM | 2900 RPM | Backwash (High) |
| Relay 2 | 19 | DI3 → COM | 2400 RPM | Day (Medium) |
| Relay 3 | 18 | DI4 → COM | 1200 RPM | Night (Low) |
| Relay 4 | 5 | -- | -- | Available |

Only one digital input should be active at a time.

## Getting Started

### Prerequisites

- [ESP-IDF v5.4](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- LilyGO T-Relay ESP32 board

### Build and Flash

```bash
source ~/esp/v5.4/esp-idf/export.sh
idf.py build
idf.py flash monitor
```

### WiFi Configuration

Three options, checked in order on boot:

1. **NVS credentials** -- If previously saved via the web UI or BLE, used automatically on boot.
2. **Build-time Kconfig** -- Copy `sdkconfig.local.example` to `sdkconfig.local`, edit WiFi SSID/password, rebuild.
3. **BLE provisioning** -- Connect to "PoolPump-ESP32" via a BLE app and write SSID to characteristic 0xFF01 and password to 0xFF02 (GATT service 0x00FF).

Once connected, navigate to `http://<device-ip>/wifi` to change WiFi credentials at any time without reflashing.

## Architecture

### Boot Sequence

NVS init → networking init → `config_init()` → WiFi connect (NVS or Kconfig) → `relay_control_init()` → `pump_controller_init()` → `price_fetcher_init()` → `web_server_init()` → `pump_scheduler_task` (FreeRTOS, priority 5)

### Scheduler

Runs every 60 seconds (`main/pump_scheduler.c`). Meets a daily water circulation volume target at minimum cost by selecting which 15-minute slots to run and at which speed, using Nordpool spot prices. Enforces operating hours and daily runtime bounds.

### Components

| Component | Role |
|-----------|------|
| `relay_control` | GPIO management for 4 relays |
| `pump_controller` | Pump state machine (OFF/NIGHT/DAY/BACKWASH) |
| `price_fetcher` / `price_client` | Fetches and caches hourly Nordpool spot prices |
| `optimizer` | Greedy volume-gap scheduling across 96 daily slots |
| `scheduler` | Scheduling framework and slot management |
| `wifi_manager` / `networking` | WiFi connection lifecycle and reconnect |
| `bluetooth_config` | BLE GATT server for mobile provisioning |
| `nvs_storage` / `storage` | Persistent config via ESP-IDF NVS |
| `web_server` | HTTP dashboard, JSON API, WiFi config page |
| `sensors` | Temperature/flow sensor interfaces |

### Project Structure

```
poolPumpControl/
├── main/
│   ├── app_main.c              # Boot sequence and initialization
│   └── pump_scheduler.c        # Scheduler loop (60s tick)
├── include/
│   └── config.h                # Global constants
├── components/
│   ├── relay_control/          # GPIO → relay control
│   ├── pump_controller/        # Pump state machine and modes
│   ├── price_fetcher/          # Nordpool price fetching and caching
│   ├── optimizer/              # Price-aware scheduling algorithm
│   ├── scheduler/              # Scheduling framework
│   ├── wifi_manager/           # WiFi lifecycle and reconnect
│   ├── bluetooth_config/       # BLE GATT provisioning
│   ├── nvs_storage/            # NVS persistent storage
│   ├── web_server/             # HTTP dashboard + API + WiFi config
│   └── sensors/                # Sensor interfaces
├── test/
│   ├── unit/                   # ESP32 target tests (Unity)
│   ├── integration/            # ESP32 integration tests
│   └── host/                   # Host-based tests (no ESP-IDF required)
└── docs/                       # Hardware docs, architecture diagrams
```

## Testing

### Host-Based Tests

Run on Linux/macOS without ESP-IDF. Compile real component source against mock ESP-IDF headers using fff (Fake Function Framework).

```bash
cd test/host && bash run_tests.sh
```

51 test cases across 6 test groups: relay_control, pump_controller, wifi_manager, nvs_storage, price_fetcher, web_server.

### ESP32 Target Tests

71 test cases using the Unity framework. Execute on ESP32 hardware. CI validates compilation and structure.

## CI/CD

Five GitHub Actions workflows:

| Workflow | Trigger | Purpose |
|----------|---------|---------|
| `esp32-ci.yml` | Push/PR | ESP-IDF build validation |
| `host-tests.yml` | Push/PR | Host-based unit tests (gcc, no ESP-IDF) |
| `quality-checks.yml` | Push/PR | clang-format + cppcheck |
| `test-suite.yml` | Daily + Push/PR | Test structure validation |
| `release.yml` | `v*.*.*` tags | GitHub release with firmware binaries |

## Related Projects

| Repository | Description |
|------------|-------------|
| [poolPumpMatter](https://github.com/andersCTO/poolPumpMatter) | Matter (Fan device) firmware -- exposes pump as Off/Low/Medium/High to Apple Home/Google Home/Alexa |
| [poolPumpControlHA](https://github.com/andersCTO/poolPumpControlHA) | Home Assistant firmware -- MQTT with HA auto-discovery, select entity for speed control |

Both use the same LilyGO T-Relay hardware and relay-to-inverter mapping but delegate scheduling to external systems (Matter controller or HA automations) instead of running it on-device.

## License

MIT License
