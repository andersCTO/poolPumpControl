# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32 pool pump controller firmware (C99, ESP-IDF v5.4) for a LilyGO T-Relay board (ESP32, 4MB flash) controlling an AquaForte VARIO+ II inverter. The core optimization problem: circulate a required volume of pool water every 24 hours at the lowest possible electricity cost, using Nordpool spot prices to schedule pump operation in the cheapest hours. The pump has multiple speed modes with different power draws and flow rates, so the scheduler must choose both *when* and *at what speed* to run.

## Build Commands

Requires ESP-IDF v5.4 environment. The ESP-IDF installation is at `~/esp/v5.4/esp-idf/`.

```bash
source ~/esp/v5.4/esp-idf/export.sh   # Activate ESP-IDF environment
idf.py build              # Build firmware
idf.py flash              # Flash to connected ESP32
idf.py monitor            # Serial monitor (exit: Ctrl+])
idf.py flash monitor      # Flash and immediately monitor
idf.py menuconfig         # Configure project settings (WiFi SSID, API endpoint)
idf.py fullclean          # Clean build directory completely
```

### Development environment

Development runs in WSL2 (Ubuntu) on Windows. The ESP32 is connected via USB (CH9102 serial chip, COM3 on Windows). USB passthrough to WSL via `usbipd` requires firewall port 3240 open. If USB passthrough doesn't work, flash directly from Windows PowerShell:

```powershell
# Install esptool on Windows: pip install esptool pyserial
# Erase flash (if needed): python -m esptool --chip esp32 -p COM3 erase_flash
python -m esptool --chip esp32 -p COM3 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 4MB --flash_freq 40m 0x1000 \\wsl$\Ubuntu\home\ankullen\development\poolPumpControl\build\bootloader\bootloader.bin 0x8000 \\wsl$\Ubuntu\home\ankullen\development\poolPumpControl\build\partition_table\partition-table.bin 0x10000 \\wsl$\Ubuntu\home\ankullen\development\poolPumpControl\build\pool_pump_controller.bin
# Monitor: python -m serial.tools.miniterm COM3 115200
```

### Build defaults (`sdkconfig.defaults`)

Key defaults: 4MB flash, Bluetooth enabled (Bluedroid, BLE-only, GATTS). When sdkconfig needs regeneration, delete it and run `idf.py build` to recreate from defaults.

## Code Quality

Format checking and static analysis run in CI and must pass:

```bash
# Format check (must pass, enforced in CI)
find . -path "./build" -prune -o -type f \( -name "*.c" -o -name "*.h" \) -print | xargs clang-format --dry-run --Werror --style=file

# Apply formatting
find . -path "./build" -prune -o -type f \( -name "*.c" -o -name "*.h" \) -print | xargs clang-format -i --style=file

# Static analysis
cppcheck --enable=all --std=c99 --language=c --platform=unix32 \
  --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unmatchedSuppression \
  --inline-suppr main/ components/
```

**Formatting rules** (.clang-format): LLVM base, 4-space indent, 120 column limit, attached braces, sorted includes.

## Testing

### ESP32 target tests

Tests use the Unity framework with manual mocks in `test/mocks/`. Tests execute on ESP32 hardware (not host-simulated). CI validates test compilation and structure but does not execute them.

- **Unit tests**: `test/unit/` — test_wifi_manager, test_relay_control, test_pump_controller, test_price_fetcher, test_nvs_storage
- **Integration tests**: `test/integration/` — test_pump_scheduling, test_full_system
- **71 total test cases** using `TEST_CASE()` macro

### Host-based tests

Host tests run on Linux/macOS without ESP-IDF installed. They compile real component source files against mock ESP-IDF headers using the fff (Fake Function Framework) approach.

```bash
cd test/host && bash run_tests.sh    # Build and run all host tests
```

- **Test files**: `test/host/tests/` — test_relay_control, test_pump_controller, test_wifi_manager, test_nvs_storage, test_price_fetcher, test_web_server
- **Mock headers**: `test/host/mocks/` — stateful mocks for gpio, wifi, nvs, http_client, http_server (includes `httpd_req_recv` mock for POST handler tests)
- **Vendored deps**: Unity (`test/host/unity/`), cJSON (`test/host/vendor/cjson/`), fff (`test/host/fff.h`)
- **51 test cases** across 6 test groups
- CI runs automatically via `host-tests.yml` on push/PR

## Architecture

### Boot sequence (`main/app_main.c`)

NVS init → networking init → `config_init()` → WiFi connect (NVS credentials checked first, Kconfig fallback) → `relay_control_init()` → `pump_controller_init()` → `price_fetcher_init()` → `web_server_init()` → spawns `pump_scheduler_task` (FreeRTOS task, priority 5).

### Scheduler loop (`main/pump_scheduler.c`)

Runs every 60 seconds. The scheduler's job is to meet the daily water circulation target at minimum cost by selecting which hours to run (based on Nordpool spot prices) and at which speed mode. It enforces operating hours and daily runtime bounds.

### Component layer (`components/`)

Each component has its own `CMakeLists.txt` and exposes headers under `include/pool_pump/`. Key components and their responsibilities:

| Component | Role |
|---|---|
| **relay_control** | GPIO pin management for 4 relays mapping to inverter digital inputs |
| **pump_controller** | Pump state machine (OFF/NIGHT/DAY/BACKWASH modes → relay configs) |
| **price_fetcher** / **price_client** | Fetches and caches hourly electricity spot prices |
| **wifi_manager** / **networking** | WiFi connection lifecycle |
| **bluetooth_config** | BLE GATT server for mobile app configuration (service 0x00FF) |
| **nvs_storage** / **storage** | Persistent config via ESP-IDF NVS |
| **web_server** | HTTP dashboard (`GET /`), JSON API (`GET /api/status`), WiFi config (`GET /wifi`, `POST /api/wifi`) |
| **scheduler** | Price-aware scheduling logic |
| **sensors** | Temperature/flow sensor interfaces |

### Hardware mapping

**LilyGO T-Relay 4-channel board** ([official repo](https://github.com/Xinyuan-LilyGO/LilyGo-T-Relay)):

| GPIO | Function |
|------|----------|
| 21 | Relay 1 |
| 19 | Relay 2 |
| 18 | Relay 3 |
| 5 | Relay 4 |
| 25 | Status LED |

Relay GPIOs control the AquaForte Vario+ inverter's digital inputs. Per the RB344 Vario manual (Section 5.4), connecting a digital input to COM triggers a **fixed speed**:

| Relay | GPIO | Inverter DI | Speed | Mode |
|-------|------|-------------|-------|------|
| Relay 1 | 21 | DI2 → COM | 2900 rpm | Backwash (High) |
| Relay 2 | 19 | DI3 → COM | 2400 rpm | Day (Medium) |
| Relay 3 | 18 | DI4 → COM | 1200 rpm | Night (Low) |
| Relay 4 | 5 | — | — | Available |

**Important**: Only one digital input should be active at a time. The inverter manual (`docs/rb344-vario-manual.pdf`) is the authoritative reference for speed/DI mapping. Board pinout in `docs/Lilygo t-relay esp32 pinmap.jpg`.

### BLE interface (`components/bluetooth_config/`)

GATT characteristics for mobile configuration:
- 0xFF01/0xFF02: WiFi SSID/password (write)
- 0xFF03: Pump settings (read/write)
- 0xFF04: Price thresholds (read/write)
- 0xFF05: System info (read/notify)
- 0xFF06: Notifications (notify)

API schema defined in the `api/` submodule (poolPumpControl-api).

## CI/CD

Five GitHub Actions workflows:
- **esp32-ci.yml**: Build validation with ESP-IDF
- **quality-checks.yml**: clang-format + cppcheck + documentation presence
- **test-suite.yml**: Test structure validation (daily + on push/PR)
- **host-tests.yml**: Host-based unit tests with cmake + gcc (no ESP-IDF required)
- **release.yml**: Triggered by `v*.*.*` tags, creates GitHub releases with firmware binaries

## Key Configuration

Global constants in `include/config.h`. Project-level Kconfig options in `Kconfig.projbuild`. Build defaults in `sdkconfig.defaults`.

### WiFi credentials

Three options, checked in order on boot:

1. **NVS credentials** — If previously saved via web UI (`/wifi`) or BLE, used automatically.
2. **Build-time Kconfig (for development)** — `cp sdkconfig.local.example sdkconfig.local`, edit SSID/password, rebuild.
3. **BLE provisioning** — Connect to "PoolPump-ESP32" via BLE (service 0x00FF), write SSID to 0xFF01 and password to 0xFF02.

Once connected, navigate to `http://<device-ip>/wifi` to change WiFi credentials at runtime without reflashing. The web UI saves to NVS and triggers `wifi_manager_reconnect()`.

## Related Projects

All three projects share the same LilyGO T-Relay hardware and relay-to-inverter mapping:

- **poolPumpMatter** (`~/development/poolPumpMatter`, `andersCTO/poolPumpMatter`) — Matter Fan device. Exposes pump as Off/Low/Medium/High to Apple Home/Google Home/Alexa. Uses esp-matter SDK + NimBLE commissioning. Scheduling delegated to Matter controller.
- **poolPumpHA** (`~/development/poolPumpHA`, `andersCTO/poolPumpControlHA`) — MQTT with Home Assistant auto-discovery. Uses `select` entity for speed control (Off/Low/Medium/High). Config portal via SoftAP for WiFi/MQTT setup. Scheduling delegated to HA automations.

## Known Issues

- **Duplicate relay init**: `relay_control_init()` is called both in `app_main.c` and inside `pump_controller_init()`, causing GPIOs to be initialized twice. Harmless but should be cleaned up.
- **WiFi event loop**: `wifi_manager_init()` logs `ESP_ERR_INVALID_STATE` because the default event loop is already created by the BT stack before WiFi init. WiFi still works once credentials are configured.
