PoolHeating

A DIY smart‑control system that optimises energy cost, water circulation, and comfort for an outdoor swimming‑pool installation.

Project Scope

Pool‑pump control (phase 1) — current focusTurn the pump on/off so the daily turnover volume is met during the cheapest electricity hours.

Inverter heat‑pump modulation (phase 2)Maintain the target water temperature while shaving peaks and exploiting solar‑PV surplus.

Everything runs on inexpensive ESP32 hardware (LilyGO T‑Relay board) with open‑source firmware that you compile and flash yourself.

Key Features (phase 1)

Feature

Status

Automatic Wi‑Fi connection (WPA2)

✅

24‑hour day‑ahead spot‑price download (Nord Pool / Tibber API)

✅

On‑device price cache & fallback to last good data

✅

User‑configurable daily turnover volume (m³)

✅

Optimised pump schedule that obeys min‑run‑time and spread constraints

✅

Manual‑override & safety time‑outs

✅

Prometheus‑compatible metrics endpoint

🔄 (beta)

Why 24‑hour prices?Nord Pool publishes tomorrow’s hourly prices at ~13:15 CET every day. Grabbing these lets the firmware compute the cheapest runtime pattern before the new day starts.

Hardware Stack

Layer

Part

Notes

Control MCU

LilyGO T‑Relay v2.0

ESP32 ‑ 8 MB Flash, two 10 A relays

VFD (pump driver)

Aqua Forte VARIO+ II

0‑10 V input used for frequency selection

Circulation Pump

Your 230 V pool pump

Up to 1.5 kW recommended

The T‑Relay’s Relay‑1 is wired to the VARIO+ II’s RUN/STOP terminal. Full wiring diagram is in /docs/wiring.svg.

Firmware Quick Start

# 1. Clone repo & init submodules
$ git clone https://github.com/youruser/PoolHeating.git
$ cd PoolHeating && git submodule update --init --recursive

# 2. Set your secrets
$ cp firmware/config.example.h firmware/config.h
# ↪ edit WIFI_SSID, WIFI_PASS, PRICE_API_TOKEN, HOME_CURRENCY

# 3. Build and flash (uses PlatformIO)
$ pio run -e t_relay -t upload

The firmware boots, joins Wi‑Fi, fetches the latest 24‑h price table, then turns the pump on/off according to the computed schedule.

Electricity‑Price Data Sources

By default the firmware queries Nord Pool’s day‑ahead API for your bidding area (SE 1‑4, DK 1‑2, FI, etc.). If you have a Tibber subscription, enable USE_TIBBER and provide your TIBBER_TOKEN to get prices in your contract’s currency including VAT.

Source

Pros

Cons

Nord Pool public API

Free, no auth

VAT‑less, JSON format changes sometimes

Tibber GraphQL

Includes VAT & fees, stable schema

Requires subscription

Scheduling Algorithm

Sort tomorrow’s 24 price points by cost ascending.

Pick the cheapest N hours whose combined flow × time ≥ daily turnover target.

Enforce a minimum rest interval between runs (to avoid short‑cycling).

Re‑sort the chosen hours chronologically and emit an RTC wake‑up list.

This greedy algorithm gives >97 % of the theoretical optimum while fitting in SRAM.

Roadmap



Safety & Compliance

Always follow the Aqua Forte VARIO+ II manual’s wiring instructions and local electrical codes. The VFD must be earthed, and relay wiring should be in a separate low‑voltage conduit. The manual is included in /docs/ for reference.

License

MIT — see LICENSE file.
