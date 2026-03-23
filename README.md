# esphome-pi18

ESPHome external component for inverters using the **PI18 protocol** (Easun, InfiniSolar, Voltronic).

Tested on: **Easun iGrid SV IV 5.6 kW** in 3-phase parallel (3× units).

## Features

- Full PI18 protocol parser (CSV-based, no sscanf)
- Correct CRC-16/CCITT implementation
- All read commands: GS, PIRI, FWS, MOD, FLAG, ET, PGSn
- All write commands via switches
- Built-in hardware watchdog (GPIO toggle) to prevent ESP8266 WDT reset
- Parallel/3-phase support: configurable number of units (1–3)

## Wiring

```
Inverter RS232 (RJ45)  →  MAX3232  →  ESP8266
TX  →  RX (GPIO3)
RX  →  TX (GPIO1)
GND →  GND
```

## Usage

```yaml
external_components:
  - source: github://JakubRybakowski/esphome-pi18@main
    refresh: 0s

uart:
  baud_rate: 2400
  tx_pin: GPIO1
  rx_pin: GPIO3

pi18:
  id: inverter
  uart_id: uart_0
  update_interval: 5s
  watchdog_pin: GPIO13
  watchdog_interval: 1000ms
  parallel_units: 3   # 1 for single unit, 2 or 3 for parallel

sensor:
  - platform: pi18
    pi18_id: inverter
    pgs_total_ac_output_active_power:
      name: "Total AC Power"
    pgs_battery_voltage:
      name: "Battery Voltage"
    # ... see test/easun.yaml for full example
```

## Commands reference

| Command | Description |
|---------|-------------|
| `^P005GS` | General status (voltage, current, power, temp) |
| `^P007PIRI` | Rated parameters |
| `^P005FWS` | Fault & warning status |
| `^P006MOD` | Working mode |
| `^P007FLAG` | Feature flags |
| `^P005ET` | Total generated energy |
| `^P007PGSn` | Parallel group status (n = unit 0–2) |

## Known issues

In 3-phase parallel mode, `^P005GS` may return all zeros on some firmware versions.
Use `pgs_*` sensors (from `^P007PGSn`) which work reliably in all modes.
