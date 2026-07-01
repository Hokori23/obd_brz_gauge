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
- Temperature units now display as `°C` for:
  - `CLT`
  - `IAT`
  - `OIL`
  - `BKT`
- Delete-confirm buttons were widened and laid out as a two-column grid to avoid text overflow on the round screen.

## Key findings

- The brake-temperature RS485 task does not need to run continuously; tying it to visible UI demand and RaceChrono subscription demand is enough and avoids unnecessary background polling.
- For the round WS175 display, dashboard layout quality improves when row/card spans are derived from the circular safe area instead of fixed percentages alone.
- `°C` should be emitted from shared display metadata, not patched at individual screen labels.

## Verification

- Source-level verification and targeted diff review completed.
- `idf.py build` was not available in the current terminal session, so no local compile confirmation was performed in this round.

## Next time

- Build once the ESP-IDF environment is active in the shell, then do a hardware pass for:
  - 1-slot and 4-slot dashboard balance
  - dashboard-config screen fit on device
  - `°C` visual balance against the current unit font
