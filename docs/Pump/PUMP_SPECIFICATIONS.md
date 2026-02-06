# Pump Specifications

## Hardware Setup

The pool pump system consists of two components:
1. **Pump motor**: Hydro-S SS075 (single-speed motor)
2. **Inverter/VFD**: AquaForte RB344 Vario (variable frequency drive)

The inverter controls the pump speed via digital inputs, allowing variable speed operation from a single-speed motor.

---

## Hydro-S SS075 Pump Motor

### Nameplate Data (from sticker)

| Parameter | Value |
|-----------|-------|
| Model | SS075 |
| Brand | Hydro-S |
| Type | Pool & Spa Pump |
| Horsepower | 0.75 HP |
| Input Power | 0.75 kW (750W) |
| Voltage | 230V~ |
| Frequency | 50 Hz |
| RPM | 2800 |
| Protection | IP55 |
| Insulation Class | B |
| Serial | MEPU12122031 |

### Flow Rate vs Head Pressure

From the Hydro-S SS series specification table (at full speed, 50Hz):

| Model | Input (kW) | Current (A) | Noise (dB) | 2m head | 4m head | 6m head | 8m head |
|-------|------------|-------------|------------|---------|---------|---------|---------|
| SS020 | 0.28 | 1.5 | 55 | 8.4 m³/h | 4 m³/h | -- | -- |
| SS033 | 0.43 | 2 | 55 | 10.4 m³/h | 7 m³/h | -- | -- |
| SS050 | 0.55 | 2.5 | 60 | 12.4 m³/h | 10 m³/h | 4 m³/h | -- |
| **SS075** | **0.75** | **3.5** | **60** | **14.4 m³/h** | **12.2 m³/h** | **9.4 m³/h** | **5 m³/h** |
| SS100 | 0.90 | 4.7 | 65 | 15.2 m³/h | 13.8 m³/h | 11.4 m³/h | 8.4 m³/h |
| SS120 | 0.97 | 5.8 | 65 | 18 m³/h | 16 m³/h | 14 m³/h | 12 m³/h |

### SS075 Flow Rates (converted to L/h)

| Head Pressure | Flow Rate |
|---------------|-----------|
| 2m | 14,400 L/h |
| 4m | 12,200 L/h |
| 6m | 9,400 L/h |
| 8m | 5,000 L/h |

**Note**: Actual flow depends on total dynamic head (TDH) of the system, which includes:
- Filter resistance
- Pipe friction losses
- Height difference between pool water level and pump
- Fitting losses

A typical pool system has 4-6m TDH.

---

## AquaForte RB344 Vario Inverter

The inverter controls pump speed via digital inputs. Per the RB344 Vario manual (Section 5.4), connecting a digital input to COM triggers a fixed speed:

| Digital Input | Speed | Relay Mapping |
|---------------|-------|---------------|
| DI2 → COM | 2900 RPM | Relay 1 (GPIO 21) |
| DI3 → COM | 2400 RPM | Relay 2 (GPIO 19) |
| DI4 → COM | 1400 RPM | Relay 3 (GPIO 18) |

**Important**: Only one digital input should be active at a time.

---

## Empirically Measured Values

Power consumption measured from AquaForte inverter display:

| Mode | RPM | Power (W) | Source |
|------|-----|-----------|--------|
| Low | 1400 | 88 | **Measured** |
| Medium | 2400 | 353 | Interpolated |
| High | 2900 | 486 | **Measured** |

**Notes:**
- Power values are significantly lower than nameplate (750W) due to actual system load
- The measured values do not follow pure cube-law scaling, which is normal for real VFD systems
- Medium speed power (353W) is interpolated between measured low and high values

---

## Flow Rate Estimates

Flow rates have NOT been empirically measured. Estimates based on pump curves and affinity laws:

### Theoretical Calculations

Using pump affinity laws:
- Flow scales linearly with speed: Q2 = Q1 × (N2/N1)

**At 6m head (conservative estimate for typical pool system):**

Base flow at full speed (2800 RPM, 6m head): 9,400 L/h (from spec sheet)

| Mode | RPM | Speed Ratio | Estimated Flow (L/h) |
|------|-----|-------------|----------------------|
| High | 2900 | 103.6% | ~9,700 |
| Medium | 2400 | 85.7% | ~8,050 |
| Low | 1400 | 50.0% | ~4,700 |

### Firmware Configuration Values

Based on empirical power measurements and conservative flow estimates (~4m head):

| Mode | RPM | Flow (L/h) | Power (W) | Source |
|------|-----|------------|-----------|--------|
| Low | 1400 | 5,000 | 88 | Power: measured, Flow: conservative |
| Medium | 2400 | 8,000 | 353 | Power: interpolated, Flow: estimated |
| High | 2900 | 12,000 | 486 | Power: measured, Flow: conservative |

**Reasoning:**
- Power values are empirically measured from AquaForte inverter display
- Flow values assume ~4m system head (conservative for typical pool)
- Measured power (486W) is below nameplate (750W), suggesting actual head may be lower (2-3m)
- Conservative flow estimates ensure adequate runtime for water quality

---

## Source Documents

- `68e7bdd0-c706-425b-b674-bd134c0b1f30.jpg` - Photo of pump nameplate sticker
- `hydro-s_spc.jpg` - Product specification table
- `MEGA-SS-series-20130325.pdf` - Full installation and operating manual
- `../rb344-vario-manual.pdf` - Inverter manual (in parent docs folder)

---

## Pool Configuration

| Parameter | Value |
|-----------|-------|
| Pool Volume | 60,000 L (60 m³) |
| Configured Circulation | 1x per day (60,000 L target) |

**Circulation guidelines:**
- 1x turnover: Normal operation, good chemical balance (current setting)
- 1.5x turnover: Heavy use, hot weather
- 2x turnover: Very heavy use, algae prevention

**Runtime estimates at 1x turnover (60,000 L/day):**
- Low speed only: 60,000 / 5,000 = 12 hours
- High speed only: 60,000 / 12,000 = 5 hours
- Mixed (optimizer): Typically 6-10 hours depending on price optimization

---

## Electricity Pricing

### Spot Price
- Source: elprisetjustnu.se API (Nordpool SE3 area)
- Updates: Hourly prices, fetched daily
- Includes: VAT (moms)

### Additional Costs (Mälarenergi Elnät)

On top of the spot price, the following costs apply:

| Cost Component | Amount | Notes |
|----------------|--------|-------|
| Elöverföringsavgift (grid fee) | ~21.5 öre/kWh | Mälarenergi 2026 rates |
| Energiskatt (energy tax) | ~54 öre/kWh | Swedish energy tax |
| Other fees | ~25 öre/kWh | Certificates, retailer margin |
| **Total additional** | **~1.00 SEK/kWh** | Configured in firmware |

**Total electricity cost = Spot price + 1.00 SEK/kWh**

Sources:
- [Mälarenergi Elnätspriser](https://www.malarenergi.se/el/elnat/priser-elnat/)

---

*Document created: 2026-02-06*
*Based on hardware inspection and manufacturer documentation*
