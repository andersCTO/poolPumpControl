@echo off
REM Flash ESP32 firmware from Windows
REM Usage: .\flash.bat [COM_PORT]
REM Default port: COM3

set PORT=%1
if "%PORT%"=="" set PORT=COM3

echo Flashing to %PORT%...
python -m esptool --chip esp32 -p %PORT% -b 460800 --before default-reset --after hard-reset write-flash --flash-mode dio --flash-size 4MB --flash-freq 40m 0x1000 \\wsl$\Ubuntu\home\ankullen\development\poolPumpControl\build\bootloader\bootloader.bin 0x8000 \\wsl$\Ubuntu\home\ankullen\development\poolPumpControl\build\partition_table\partition-table.bin 0x10000 \\wsl$\Ubuntu\home\ankullen\development\poolPumpControl\build\pool_pump_controller.bin

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Flash complete! Starting monitor...
    echo Press Ctrl+] to exit monitor
    echo.
    python -m serial.tools.miniterm %PORT% 115200
) else (
    echo.
    echo Flash failed!
)
