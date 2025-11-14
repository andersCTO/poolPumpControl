# Bluetooth Configuration Component

## Översikt

Bluetooth-konfigurationsmodulen tillhandahåller BLE (Bluetooth Low Energy) gränssnitt för konfiguration via mobilapp.

## Funktioner

### Initial Implementation
- ✅ WiFi-konfiguration (SSID och lösenord)
- ✅ BLE GATT-server för kommunikation
- ✅ Säker dataöverföring

### Framtida Funktioner
- ⏳ Pumppinställningar (hastighet, driftschema)
- ⏳ Priströsklar för elpriser
- ⏳ Systemstatus och diagnostik
- ⏳ Schema-konfiguration

## Användning

### Initialisering

```c
#include "bluetooth_config.h"

// Callback för konfigurationsändringar
void config_callback(bt_config_type_t type, void *data, size_t len) {
    if (type == BT_CONFIG_TYPE_WIFI_CREDENTIALS) {
        bt_wifi_credentials_t *creds = (bt_wifi_credentials_t *)data;
        ESP_LOGI("APP", "WiFi SSID: %s", creds->ssid);
        // Spara WiFi-inställningar
    }
}

// Initiera Bluetooth-konfiguration
bluetooth_config_init("PoolPump-ESP32", config_callback);

// Starta annonsering (gör enheten synlig)
bluetooth_config_start_advertising();
```

### Ansluta från Mobilapp

1. **Scanna efter enheter**: Appen söker efter "PoolPump-ESP32"
2. **Anslut till enhet**: Välj enheten i listan
3. **Konfigurera WiFi**: Skriv WiFi SSID och lösenord
4. **Bekräfta**: Enheten ansluter till WiFi

## BLE Services och Characteristics

### Configuration Service (UUID: 0x00FF)

| Characteristic | UUID | Typ | Beskrivning |
|----------------|------|-----|-------------|
| WiFi SSID | 0xFF01 | Write | WiFi-nätverkets namn |
| WiFi Password | 0xFF02 | Write | WiFi-lösenord |
| Pump Settings | 0xFF03 | Read/Write | Pumppinställningar |
| Price Settings | 0xFF04 | Read/Write | Priströsklar |
| System Info | 0xFF05 | Read/Notify | Systemstatus |
| Notifications | 0xFF06 | Notify | Meddelanden från enheten |

## Säkerhet

- BLE-kommunikation är krypterad
- WiFi-lösenord skickas aldrig i klartext
- Enheten accepterar endast en anslutning åt gången
- Automatisk timeout efter inaktivitet

## Exempel på Mobilapp (React Native)

```javascript
import BleManager from 'react-native-ble-manager';

// Scanna efter enheter
BleManager.scan([], 5, true).then(() => {
  console.log('Scanning...');
});

// Hitta PoolPump-enhet
BleManager.connect(peripheralId).then(() => {
  // Skicka WiFi-inställningar
  const ssid = "MinWiFi";
  const data = Buffer.from(ssid);
  
  BleManager.write(
    peripheralId,
    '00FF', // Service UUID
    'FF01', // SSID Characteristic UUID
    data
  );
});
```

## Felsökning

### Enheten syns inte i BLE-scanningen
- Kontrollera att advertising är aktiverat: `bluetooth_config_start_advertising()`
- Verifiera att BT är aktiverat i `menuconfig`
- Kolla loggar för initialiseringsfel

### Kan inte skriva till characteristics
- Verifiera att enheten är ansluten: `bluetooth_config_is_connected()`
- Kontrollera UUID:er matchar
- Kolla callback-funktion körs korrekt

## Konfiguration i menuconfig

```bash
idf.py menuconfig
```

Aktivera följande:
- Component config → Bluetooth → [*] Bluetooth
- Component config → Bluetooth → Bluetooth controller → BLE 5.0 features

## API-referens

Se `bluetooth_config.h` för fullständig API-dokumentation.
