# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32 pool pump controller firmware (C99, ESP-IDF v5.1.2) for a LilyGO T-Relay board controlling an AquaForte VARIO+ II inverter. The core optimization problem: circulate a required volume of pool water every 24 hours at the lowest possible electricity cost, using Nordpool spot prices to schedule pump operation in the cheapest hours. The pump has multiple speed modes with different power draws and flow rates, so the scheduler must choose both *when* and *at what speed* to run.

## Build Commands

Requires ESP-IDF v5.1.2 environment (`. $IDF_PATH/export.sh` or equivalent).

```bash
idf.py build              # Build firmware
idf.py flash              # Flash to connected ESP32
idf.py monitor            # Serial monitor (exit: Ctrl+])
idf.py flash monitor      # Flash and immediately monitor
idf.py menuconfig         # Configure project settings (WiFi SSID, API endpoint)
idf.py fullclean          # Clean build directory completely
```

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
- **Mock headers**: `test/host/mocks/` — stateful mocks for gpio, wifi, nvs, http_client, http_server
- **Vendored deps**: Unity (`test/host/unity/`), cJSON (`test/host/vendor/cjson/`), fff (`test/host/fff.h`)
- **47 test cases** across 6 test groups
- CI runs automatically via `host-tests.yml` on push/PR

## Architecture

### Boot sequence (`main/app_main.c`)

NVS init → networking init → `config_init()` → `wifi_manager_init()` → `relay_control_init()` → `pump_controller_init()` → `price_fetcher_init()` → `web_server_init()` → spawns `pump_scheduler_task` (FreeRTOS task, priority 5).

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
| **web_server** | HTTP dashboard and JSON status API (`GET /`, `GET /api/status`) |
| **scheduler** | Price-aware scheduling logic |
| **sensors** | Temperature/flow sensor interfaces |

### Hardware mapping

Relay GPIOs control the AquaForte inverter's digital inputs:
- Relay 1 (GPIO 21) → DI2 → Night mode (1400 RPM)
- Relay 2 (GPIO 19) → DI3 → Day mode (2000 RPM)
- Relay 3 (GPIO 18) → DI4 → Backwash mode (2900 RPM)
- Relay 4 (GPIO 5) → available

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
- **esp32-ci.yml**: Build validation with ESP-IDF v5.1.2
- **quality-checks.yml**: clang-format + cppcheck + documentation presence
- **test-suite.yml**: Test structure validation (daily + on push/PR)
- **host-tests.yml**: Host-based unit tests with cmake + gcc (no ESP-IDF required)
- **release.yml**: Triggered by `v*.*.*` tags, creates GitHub releases with firmware binaries

## Key Configuration

Global constants in `include/config.h`. Project-level Kconfig options in `Kconfig.projbuild` (WiFi SSID, API endpoint). Runtime SDK config in `sdkconfig`.
