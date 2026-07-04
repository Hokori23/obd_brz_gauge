# OBD BRZ Gauge ZC6 Runtime Notes 2026-07

## Context

Repo: `obd_brz_gauge`
Board path in active use: `WS175 AMOLED`
Main thread: ZC6 telemetry expansion, monitor-mode robustness, dashboard startup polish, and board abstraction cleanup.

## What changed

- Added boot-time persisted error-log dump behind `CONFIG_OBD_BOOT_PRINT_ERROR_LOG`.
- Added display-only bootstrap mode behind `CONFIG_OBD_BOOTSTRAP_DISPLAY_ONLY`.
- Consolidated UI debug switches into `main/export_path/ui_debug_config.h`.
- Unified oil temperature UI naming to a single `OIL` metric.
- Added independent `MAP` and `IGN` metrics.
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
- `GEAR-DERIVED` can identify forward gears from RPM/speed/profile ratios, but reverse is still not a reliable derived signal.
- Current monitor-mode contention is only between `G-force` and `GEAR-MONITOR`.
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
  - Real-world `GEAR-DERIVED` shift-edge behavior
  - WS175 real ADS1115 oil-pressure sampling over the shared board I2C path
  - Real hardware comfort / timing of the post-sweep 3-burst wait effect

## Next time

- If field debug starts, prefer `erase-flash` first to remove stale dashboard page config.
- Add targeted runtime logs when validating:
  - `GEAR-DERIVED` ratio / chosen gear
  - `0x141` monitor receive vs decode failure
- Legacy `DISP_ITEM_OILC` is intentionally kept only as a dead enum hole so old persisted slot values fail soft as `UNKNOWN` instead of crashing or migrating.
- If oil-pressure bring-up moves to hardware validation, verify ADS1115 address `0x48`, AIN0 scaling, and whether both boards should eventually expose a true shared I2C bus handle instead of only abstract register access.
- If `0x141` remains unavailable on the adapter, keep `GEAR-DERIVED` as the default usable path and treat monitor gear as optional.

## Follow-up: MAP fragile-metric recovery plan

### Context

- Real board behavior on a 2-slot `RPM + MAP` dashboard page: both metrics update a few times after page init, then freeze together.
- `RPM` alone is stable, so the likely trigger is `MAP` (`01 0B`) poisoning the shared OBD poll path rather than `RPM` itself failing.

### Key decision

- Treat `MAP` as a fragile metric at the poll-slot layer instead of as a global link-health signal.
- First implementation should not pause `MAP` polling.
- `MAP` failure must keep the last good cached value on screen.
- `MAP` failure must not participate in the self-heal / BLE reconnect trigger.

### Practical impact

- Split OBD health into:
  - transport alive: any complete ELM327 response means the session is still alive
  - payload valid: only successful metric decode updates the metric freshness
- Add a fragile-slot policy table in `elm327_ble_client.c` so later metrics with the same behavior can opt in without custom ad-hoc logic.
- For fragile slots:
  - keep polling normally
  - keep last value on failure
  - record failure for diagnostics
  - never let single-slot failure escalate to re-init or reconnect

### Verification or follow-up

- Confirm on hardware by checking that `RPM` continues updating even when `MAP` stops changing.
- Boot-time persisted error-log dump is already implemented and enabled via `CONFIG_OBD_BOOT_PRINT_ERROR_LOG=y`; use that first when live serial capture is blocked.
- On `2026-07-04`, direct `COM12` capture from Codex was blocked by `Access to the port 'COM12' is denied`, so another monitor process or tool already had the port open.
- Confirmed a separate diagnostic gap on `2026-07-04`: many ELM327 runtime faults were only printed to the console and were not persisted into the NVS error ring.
- Patched runtime persistence for these throttled ELM327 fault classes:
  - self-heal re-init
  - self-heal reconnect escalation
  - previous-response wait timeout
  - response accumulation timeout
  - `NO DATA`
  - write-char / write-event failure
- Each persisted entry now includes the last OBD poll slot name, so a later boot dump can show whether the session was failing around `MAP/BOOST`, `RPM`, `TPS`, and so on.

## Follow-up: documentation and memory workflow

### Context

- The repository `docs/` tree has accumulated implementation notes, bring-up notes, plans, and runtime findings across multiple work streams.
- The current note set is useful, but discoverability is uneven and some topics now span multiple partially overlapping documents.

### Confirmed need

- Future maintenance work should include:
  - sorting docs into clearer categories
  - merging overlapping notes where practical
  - improving the folder structure so topic lookup is faster during debugging and continuation work

### Memory / skill direction

- A future skill/agent is desired with this behavior:
  - when the agent lacks enough local conversational context
  - and the task appears related to an existing project / feature / board topic
  - it should proactively search project memory first instead of rediscovering everything from code alone
- This should be treated as a preferred workflow default for continuation-heavy work, especially in repositories like this one where durable notes already exist under `references/`.

## Global authoring rule

- Highest-priority project convention:
  - all documentation must be written in Chinese
  - all code comments must be written in Chinese
  - all commit messages must be written in Chinese
- Treat this as a global always-apply rule for this repository unless the user explicitly overrides it for a specific task.

## Follow-up: ZC6 TPMS monitor and persisted diagnostics

### Context

- The runtime branch expanded beyond `MAP / IGN / gear` and now also needs ZC6 TPMS pressure visibility on dashboard pages.
- Recent field debugging also showed that many runtime faults were still too transient to diagnose unless they were persisted into the NVS error ring.
- The repository-level `docs/` tree was also intentionally slimmed down so long-lived notes live under `references/` and the external memory index instead of drifting into duplicate in-repo copies.

### What changed

- Added ZC6 TPMS monitor-mode decode path in `elm327_ble_client.c` for CAN `0x6E2`.
- Added TPMS dashboard items:
  - `TPFL`
  - `TPFR`
  - `TPRL`
  - `TPRR`
- Added TPMS values into:
  - `obd_data_cache`
  - `ui_runtime_common.h`
  - `ui_legacy_runtime.c`
  - `ui_home_runtime.c`
  - `aux_sensor_demand.*`
- Expanded persisted runtime diagnostics:
  - boot count persisted in NVS
  - formatted error-log helper `nvs_error_log_recordf()`
  - larger error ring capacity (`64`)
  - throttled persistence for ELM327 / RS485 / touch / oil-pressure / RaceChrono faults
- Changed boot-time error-log dump behavior:
  - still keeps the larger history
  - only prints the latest `16` entries during boot to reduce startup log pressure
- Switched repository `docs/` to a migration-entry role and removed duplicated historical plan files from the repo tree.

### Key decisions or findings

- TPMS monitor is demand-driven:
  - only enabled when the active page uses TPMS items
  - only exposed for ZC6
- TPMS raw-pressure scale is auto-resolved at runtime from plausible pressure ranges instead of hard-coding a single assumption too early.
- Runtime persistent diagnostics should prefer:
  - short throttled messages
  - inclusion of the last OBD poll slot where possible
  - survival across reboot for later field forensics
- Large historical docs are better maintained in the memory/reference flow than duplicated under `docs/`.

### Verification

- Local ESP-IDF build passed on `2026-07-05`.
- Working build command on this machine:
  - PowerShell:
    - `$env:PATH = 'C:\Users\Hokori\.espressif\python_env\idf5.5_py3.11_env\Scripts;' + $env:PATH`
    - `. 'D:\esp\v5.5.4-clean\export.ps1'`
    - `idf.py build`
- Build result summary:
  - app binary size `0x67f2c0`
  - app partition free `0x180d40` (`19%`)
- No hardware validation yet for:
  - real ZC6 TPMS pressure decode correctness
  - long-run stability when TPMS monitor mode and normal PID polling alternate on the real adapter

### Next time

- On hardware, verify whether the auto-resolved TPMS scale consistently lands on the expected unit family.
- If TPMS monitor steals too much time from normal PID polling, consider:
  - longer sample reuse window
  - lower re-entry frequency
  - explicit monitor priority rules
- If field logs still miss a critical class of failures, keep extending the persisted diagnostic ring conservatively instead of reverting to console-only logging.
