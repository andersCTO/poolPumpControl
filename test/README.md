# ESP32 Pool Pump Controller - Testing Framework

This document describes the comprehensive testing framework implemented for the ESP32 Pool Pump Controller project.

## Overview

The testing framework provides both unit tests and integration tests to ensure code quality and validate functionality. It uses the Unity testing framework with CMock for mocking ESP-IDF APIs.

## Test Structure

```
test/
├── CMakeLists.txt          # Main test configuration
├── test_main.c            # Unity test runner
├── unit/                  # Unit tests (isolated component testing)
│   ├── CMakeLists.txt
│   ├── test_wifi_manager.c
│   ├── test_relay_control.c
│   ├── test_pump_controller.c
│   ├── test_price_fetcher.c
│   └── test_nvs_storage.c
├── integration/           # Integration tests (component interaction)
│   ├── CMakeLists.txt
│   ├── test_pump_scheduling.c
│   └── test_full_system.c
└── mocks/                 # Mock implementations
    ├── CMakeLists.txt
    ├── mock_esp_wifi.h/.c
    ├── mock_driver_gpio.h/.c
    └── mock_esp_http_client.h/.c
```

## Test Categories

### Unit Tests
- **test_wifi_manager.c**: Tests WiFi connection, disconnection, status monitoring
- **test_relay_control.c**: Tests GPIO relay control, initialization, state management
- **test_pump_controller.c**: Tests pump modes, start/stop operations, status reporting
- **test_price_fetcher.c**: Tests price data fetching, parsing, low-price detection
- **test_nvs_storage.c**: Tests persistent storage of schedules, settings, WiFi config

### Integration Tests
- **test_pump_scheduling.c**: Tests scheduled pump operation, price-based scheduling, backwash cycles
- **test_full_system.c**: Tests complete system integration, error recovery, performance

## Mock Framework

The framework includes mocks for ESP-IDF APIs to enable isolated testing:

- **WiFi API**: Connection states, network operations
- **GPIO Driver**: Pin control, interrupt handling
- **HTTP Client**: Network requests, response handling

## Running Tests

### On ESP32 Device

1. Build the project with tests enabled:
   ```bash
   idf.py build
   ```

2. Flash to ESP32:
   ```bash
   idf.py flash
   ```

3. Monitor test output:
   ```bash
   idf.py monitor
   ```

The tests will run automatically on device startup and output results to the serial console.

### Test Results

Test output includes:
- Test group execution
- Individual test pass/fail status
- Assertion failures with details
- Final summary with pass/fail counts

## Test Coverage

### Unit Test Coverage
- WiFi Manager: 9 test cases
- Relay Control: 11 test cases
- Pump Controller: 12 test cases
- Price Fetcher: 10 test cases
- NVS Storage: 12 test cases

### Integration Test Coverage
- Pump Scheduling: 8 test cases
- Full System: 9 test cases

Total: **71 test cases** covering all major components and interactions.

## Adding New Tests

### Unit Tests
1. Create `test_<component>.c` in `test/unit/`
2. Include necessary headers and mocks
3. Define test group with `TEST_GROUP()`
4. Implement `TEST_SETUP()` and `TEST_TEAR_DOWN()` if needed
5. Add test cases with `TEST()`
6. Add test group runner with `TEST_GROUP_RUNNER()`

### Integration Tests
1. Create `test_<feature>.c` in `test/integration/`
2. Initialize all required components in setup
3. Test component interactions
4. Clean up in teardown

### Mocks
1. Create `mock_<api>.h/.c` in `test/mocks/`
2. Implement mock functions matching ESP-IDF APIs
3. Add state tracking and control functions
4. Include in test CMakeLists.txt

## Test Configuration

Tests are configured in:
- `test/CMakeLists.txt`: Main test setup
- `test/unit/CMakeLists.txt`: Unit test components
- `test/integration/CMakeLists.txt`: Integration test components
- `test/mocks/CMakeLists.txt`: Mock components

## Dependencies

- Unity: Unit testing framework
- CMock: Mock generation (optional, manual mocks used)
- ESP-IDF test runner
- All project components

## Best Practices

1. **Isolation**: Use mocks to test components independently
2. **Setup/Teardown**: Initialize and clean up test state
3. **Assertions**: Use appropriate Unity assertions
4. **Coverage**: Test normal operation, error conditions, edge cases
5. **Documentation**: Comment test purpose and expected behavior

## Future Enhancements

- Code coverage analysis
- Automated CI/CD integration
- Hardware-in-the-loop testing
- Performance benchmarking
- Memory leak detection