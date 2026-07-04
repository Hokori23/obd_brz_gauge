# OBD BRZ Gauge ZC6 Runtime Notes 2026-07

## Context

Repo: `obd_brz_gauge`
Board path in active use: `WS175 AMOLED`
Main thread: ZC6 telemetry expansion, monitor-mode robustness, dashboard startup polish, and board abstraction cleanup.

## What changed

- Added boot-time persisted error-log dump behind `CONFIG_OBD_BOOT_PRINT_ERROR_LOG`.
- Added display-only bootstrap mode behind `CONFIG_OBD_BOOTSTRAP_DISPLAY_ONLY`.
- Consolidated UI debug switches into `main/export_path/ui_debug_config.h`.
- Split oil temperature into two runtime metrics:
  - `OIL-PID`
  - `OIL-CAN`
- Added independent `MAP` and `IGN` metrics.
- Added ZC6 low-frequency CAN oil-temp monitor on `0x360`.
- Kept ZC6 gear monitor on `0x141` and renamed the computed gear path to `GEAR-DERIVED`.
- Upgraded derived-gear logic to use profile ratio ranges plus short hysteresis.
- Increased poll weighting:
  - `LOD=4`
  - `TPS=4`
  - `Speed=3`
- Added stronger ELM327 self-heal and reconnect behavior.
- Added a first-valid-data refresh hook from `elm327_ble_client` into the home UI so the active tile redraws as soon as real OBD data lands after boot/connect.
- Replaced the old waiting brightness pulse with a Defi-style 3-burst hardware brightness effect based on the saved `brightness_day` setting:
  - burst low point = 30% of configured brightness
  - after the third burst, the display stays dim while waiting for the first valid OBD payload
  - once the first valid payload arrives, brightness returns to the configured baseline
- Added board-level `board_i2c_reg_read()` / `board_i2c_reg_write()` helpers so upper-layer sensor code no longer depends on a board-specific I2C driver API.
- Re-enabled shared `ads1115_oil_pressure.c` for `WS175` and routed both `WS175` and `WS185` oil-pressure register access through the same board abstraction.

## Key findings

- `MAP` can be sourced directly from standard OBD PID `01 0B`.
- `IGN` can be sourced directly from standard OBD PID `01 0E`.
- ZC6 `OIL-CAN` is practical as a low-frequency monitor-mode feature because temperature does not need fast refresh.
- `GEAR-DERIVED` can identify forward gears from RPM/speed/profile ratios, but reverse is still not a reliable derived signal.
- `GEAR-MONITOR` and `OIL-CAN` both consume ELM327 monitor mode, so they must remain mutually exclusive at runtime.
- `GEAR-MONITOR` still uses an extra flag in NVS because page type storage only has 2 raw bits.
- BLE-connected state is earlier than real-data-ready state. Sweep completion alone is not a safe point to assume visible metrics will refresh without an explicit first-valid-data bridge.
- Full Defi-style light cut is a poor fit for an electronic display because the content disappears with the light; a reduced-brightness burst gives the same waiting cue without turning the screen into a black strobe.
- `WS175` and `WS185` are mostly unified at the app layer now, but capability-specific differences like display rotation still intentionally remain board-gated in the UI.

## Verification

- Local ESP-IDF build passed on `2026-07-04`.
- Verified build command pattern on this machine:
  - `python.exe D:\\esp\\v5.5.4-clean\\tools\\activate.py --export`
  - `. <exported-ps1>`
  - `idf.py build`
- Build still passed after:
  - adding board-level I2C register helpers
  - re-enabling shared ADS1115 oil-pressure code for `WS175`
  - adding the first-valid-data home refresh hook
  - switching the waiting state to a 3-burst brightness sequence
- No hardware validation yet for:
  - ZC6 `0x141` monitor on current adapter
  - ZC6 `0x360` oil-temp monitor on current adapter
  - Real-world `GEAR-DERIVED` shift-edge behavior
  - WS175 real ADS1115 oil-pressure sampling over the shared board I2C path
  - Real hardware comfort / timing of the post-sweep 3-burst wait effect

## Next time

- If field debug starts, prefer `erase-flash` first to remove stale dashboard page config.
- Add targeted runtime logs when validating:
  - `GEAR-DERIVED` ratio / chosen gear
  - `0x141` monitor receive vs decode failure
  - `0x360` monitor receive vs decode failure
- If oil-pressure bring-up moves to hardware validation, verify ADS1115 address `0x48`, AIN0 scaling, and whether both boards should eventually expose a true shared I2C bus handle instead of only abstract register access.
- If `0x141` remains unavailable on the adapter, keep `GEAR-DERIVED` as the default usable path and treat monitor gear as optional.
