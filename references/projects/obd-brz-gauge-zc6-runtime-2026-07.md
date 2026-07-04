# OBD BRZ Gauge ZC6 Runtime Notes 2026-07

## Context

Repo: `obd_brz_gauge`
Board path in active use: `WS175 AMOLED`
Main thread: ZC6 telemetry expansion, monitor-mode robustness, and dashboard debug cleanup.

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
- Stopped hard-coding WS175 as “no oil pressure support” at bootstrap; board implementation still decides actual sensor availability.

## Key findings

- `MAP` can be sourced directly from standard OBD PID `01 0B`.
- `IGN` can be sourced directly from standard OBD PID `01 0E`.
- ZC6 `OIL-CAN` is practical as a low-frequency monitor-mode feature because temperature does not need fast refresh.
- `GEAR-DERIVED` can identify forward gears from RPM/speed/profile ratios, but reverse is still not a reliable derived signal.
- `GEAR-MONITOR` and `OIL-CAN` both consume ELM327 monitor mode, so they must remain mutually exclusive at runtime.
- `GEAR-MONITOR` still uses an extra flag in NVS because page type storage only has 2 raw bits.

## Verification

- Local ESP-IDF build passed on `2026-07-04`.
- Verified build command pattern on this machine:
  - `python.exe D:\\esp\\v5.5.4-clean\\tools\\activate.py --export`
  - `. <exported-ps1>`
  - `idf.py build`
- No hardware validation yet for:
  - ZC6 `0x141` monitor on current adapter
  - ZC6 `0x360` oil-temp monitor on current adapter
  - Real-world `GEAR-DERIVED` shift-edge behavior

## Next time

- If field debug starts, prefer `erase-flash` first to remove stale dashboard page config.
- Add targeted runtime logs when validating:
  - `GEAR-DERIVED` ratio / chosen gear
  - `0x141` monitor receive vs decode failure
  - `0x360` monitor receive vs decode failure
- If `0x141` remains unavailable on the adapter, keep `GEAR-DERIVED` as the default usable path and treat monitor gear as optional.
