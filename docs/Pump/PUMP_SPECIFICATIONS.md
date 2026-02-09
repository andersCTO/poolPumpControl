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

Based on empirical power measurements and affinity law calculations:

| Mode | RPM | Flow (L/h) | Flow/slot | Power (W) | Source |
|------|-----|------------|-----------|-----------|--------|
| Low | 1400 | 5,000 | 1,250 L | 88 | **Measured** |
| Medium | 2400 | 8,500 | 2,125 L | 320 | Affinity laws |
| High | 2900 | 10,400 | 2,600 L | 486 | **Measured** |

**Affinity Law Calculations:**

Power was fitted to measured points (88W @ 1400 RPM, 486W @ 2900 RPM):
- Power ratio: 486/88 = 5.52
- RPM ratio: 2900/1400 = 2.07
- Exponent: ln(5.52)/ln(2.07) = 2.34
- Formula: P = 88 × (RPM/1400)^2.34

Flow scales linearly with RPM:
- Formula: Q = 5000 × (RPM/1400)

**Efficiency (Liters per Watt-hour):**

| Mode | Efficiency | Rank |
|------|------------|------|
| Low | 56.8 L/Wh | Best |
| Medium | 26.6 L/Wh | Middle |
| High | 21.4 L/Wh | Worst |

This confirms physical expectation: faster = less efficient per liter.

---

## Source Documents

- `68e7bdd0-c706-425b-b674-bd134c0b1f30.jpg` - Photo of pump nameplate sticker
- `hydro-s_spc.jpg` - Product specification table
- `MEGA-SS-series-20130325.pdf` - Full installation and operating manual
- `../rb344-vario-manual.pdf` - Inverter manual (in parent docs folder)

---

---

## Schedule Optimization Algorithm

### Problem Formulation (MILP)

The pump scheduling problem is formulated as Mixed-Integer Linear Programming:

**Decision Variables:**
- `x[i][m]` ∈ {0,1} — binary: slot i uses mode m

**Modes:**
- m=0: OFF (0 L, 0 W)
- m=1: LOW (1,250 L, 88 W)
- m=2: MEDIUM (2,125 L, 320 W)
- m=3: HIGH (2,600 L, 486 W)

**Objective Function:**
```
minimize Σᵢ Σₘ (power[m] × 0.25h × (spot_price[i] + additional_cost) × x[i][m])
```

**Constraints:**
1. One mode per slot: `Σₘ x[i][m] = 1` for all slots i
2. Meet volume target: `Σᵢ Σₘ (volume[m] × x[i][m]) ≥ target_volume`
3. Binary: `x[i][m] ∈ {0, 1}`

### Solution Method: Dynamic Programming

This is a **Multiple-Choice Knapsack Problem** — solvable optimally with DP.

**State:** `dp[i][v]` = minimum cost to achieve volume v using slots 0..i-1

**Transitions:** For each slot i, try all 4 modes:
```
dp[i+1][v + volume[m]] = min(dp[i+1][v + volume[m]],
                              dp[i][v] + cost[i][m])
```

**Complexity:**
- Slots: 96
- Volume states: ~1,500 (scaled by GCD)
- Modes: 4
- Total: O(96 × 1500 × 4) ≈ 576K operations
- Memory: ~300 KB (fits in ESP32)

### Why Not Greedy?

The previous greedy algorithm (fill LOW, then upgrade to HIGH) was suboptimal because:

1. It ignored MEDIUM mode entirely
2. It didn't consider that adding a new LOW slot might be cheaper than upgrading existing slot
3. Different price slots have different optimal mode choices

**Example:** At a very cheap price slot, HIGH mode might be cost-effective. At expensive slots, only LOW (or OFF) makes sense.

### Marginal Cost Analysis

For slot with price p, cost per marginal liter:

| Transition | Extra Volume | Extra Cost | Marginal ¢/L |
|------------|--------------|------------|--------------|
| OFF→LOW | 1,250 L | 22p | 1.76p |
| OFF→MEDIUM | 2,125 L | 80p | 3.76p |
| OFF→HIGH | 2,600 L | 121.5p | 4.67p |
| LOW→MEDIUM | 875 L | 58p | 6.63p |
| LOW→HIGH | 1,350 L | 99.5p | 7.37p |
| MEDIUM→HIGH | 475 L | 41.5p | 8.74p |

**Key insight:** OFF→LOW is always most efficient. Upgrades have diminishing returns.

---

## Pool Configuration

| Parameter | Value |
|-----------|-------|
| Pool Volume | 60,000 L (60 m³) |
| Configured Circulation | 3x per day (180,000 L target) |

**Circulation guidelines:**
- 1x turnover: Minimum for chemical balance
- 2x turnover: Normal operation
- 3x turnover: Required for heating system (current setting)
- 4x turnover: Heavy use, very hot weather

**Runtime estimates at 3x turnover (180,000 L/day):**
- Max LOW capacity: 96 slots × 1,250 L = 120,000 L (insufficient)
- Shortfall: 60,000 L → requires ~35 HIGH upgrades
- Expected schedule: ~61 LOW slots + ~35 HIGH slots (24h runtime)
- The optimizer picks cheapest slots for HIGH upgrades

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
