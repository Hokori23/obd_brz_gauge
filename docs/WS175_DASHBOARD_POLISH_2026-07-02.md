# WS175 Dashboard Polish 2026-07-02

## Context

This round focused on the dynamic dashboard home and its related drill-in screens on the WS175 round display.

## What changed

- Added `aux_sensor_demand_refresh()` and wired it into app startup, home-page changes, dashboard config, BLE scan, settings, and RaceChrono filter updates.
- RS485 brake temperature polling is now demand-driven:
  - enabled when the active home page uses `BKT`
  - enabled when RaceChrono requests the brake-temp PID
  - otherwise held idle
- Home-page dashboard cards now use geometry-driven layout and typography instead of older fixed-position assumptions.
- Dashboard config layout now computes safe spans against the round screen and no longer relies on the old absolute row table.
- The dashboard config return path now maps back to the actual gauge page instead of assuming `page_id - 1`.
- BLE scan and settings now use the same enter/exit gesture direction contract.
- Removed instructional hint copy from dashboard-related pages, including:
  - `Swipe up to go back`
  - `ADD PAGE`
- Temperature units now display as `掳C` for:
  - `CLT`
  - `IAT`
  - `OIL`
  - `BKT`
- Delete-confirm buttons were widened and laid out as a two-column grid to avoid text overflow on the round screen.
- Home refresh cadence is now asymmetric:
  - the active tile still refreshes every timer tick
  - mounted neighbor tiles refresh at a lower cadence for swipe smoothness without paying the full 3-tile refresh cost every 200 ms
- NVS background flush no longer holds `s_mux` across `save_blob()`:
  - stats and diagnostic error logs are snapshotted under lock
  - the actual NVS write happens outside the critical section
  - dirty flags are cleared only if no newer write arrived during the flush window
- Added regression contracts for:
  - lazy home metric reads
  - neighbor-tile refresh cadence
  - NVS flush contention
  - persisted error-log background flush flow

## Key findings

- The brake-temperature RS485 task does not need to run continuously; tying it to visible UI demand and RaceChrono subscription demand is enough and avoids unnecessary background polling.
- For the round WS175 display, dashboard layout quality improves when row/card spans are derived from the circular safe area instead of fixed percentages alone.
- `掳C` should be emitted from shared display metadata, not patched at individual screen labels.
- On this hardware/software stack, the next meaningful UI-performance win is reducing unnecessary LVGL text/layout work, not pushing the WS175 display path toward risky full-frame PSRAM buffering.
- For storage writes, lock hold time matters more than raw flush frequency; moving NVS I/O out of the mutex is a better ROI optimization than simply stretching the timer further.

## Verification

- `powershell -ExecutionPolicy Bypass -File .\tools\run_ui_platform_static_tests.ps1`
- `idf.py build`
- The static test bundle now includes the new refresh-cadence and NVS-flush contention contracts, so later refactors have a higher chance of catching silent performance regressions.

## Next time

- Consider page-type-aware refresh periods:
  - `G-force` pages may justify a faster cadence
  - menu/static pages can likely run slower
- Extract more dashboard-config rules into host-testable pure logic so later page-type expansion stays cheaper to verify.
- Keep the WS175 line-buffer strategy unless a dedicated hardware revalidation proves full-frame buffering is actually safer and faster on-device.
