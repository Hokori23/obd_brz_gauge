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
- Home refresh cadence is now active-page-only:
  - only the active tile performs periodic value refresh
  - swipe-driven page changes still trigger an immediate refresh on the newly active page
  - neighbor pages can remain mounted for gesture smoothness, but they no longer keep polling or refreshing data in the background
- Home refresh cadence is now also page-type-aware:
  - menu / add pages run slower
  - metric pages keep the previous balanced cadence
  - gear and G-force pages run faster to improve perceived responsiveness
- Swipe-driven page changes now refresh both:
  - the active sensor-demand mask
  - the home refresh timer profile
  This closes the gap where gesture navigation could previously keep polling rules from the old page for longer than intended.
- NVS background flush no longer holds `s_mux` across `save_blob()`:
  - stats and diagnostic error logs are snapshotted under lock
  - the actual NVS write happens outside the critical section
  - dirty flags are cleared only if no newer write arrived during the flush window
- OBD PID polling now skips empty schedule gaps instead of paying one `slot_delay` per skipped slot:
  - the scheduler jumps straight to the next demanded PID slot
  - repeated RPM / speed / TPS weighting still stays intact
  - when no OBD metric is currently demanded, the task falls back to a slower idle delay instead of spinning through the full schedule
- Added regression contracts for:
  - lazy home metric reads
  - active-page-only home refresh cadence
  - settings hierarchy and round-safe layout
  - dashboard-config fallback navigation
  - NVS flush contention
  - persisted error-log background flush flow
  - demanded OBD poll-slot scheduling
- Settings is now a unified horizontal navigation page:
  - peer categories: `DISPLAY` / `DASHBOARD` / `VEHICLE` / `OBD`
  - each category completes interaction in one screen
  - page switching and safe-area layout are computed against the round display instead of the earlier absolute-row form
- `No page config` remains as an explicit fallback screen, but it now exposes a `BACK` action and returns to the originating gauge page when possible.

## Key findings

- The brake-temperature RS485 task does not need to run continuously; tying it to visible UI demand and RaceChrono subscription demand is enough and avoids unnecessary background polling.
- For the round WS175 display, dashboard layout quality improves when row/card spans are derived from the circular safe area instead of fixed percentages alone.
- `掳C` should be emitted from shared display metadata, not patched at individual screen labels.
- On this hardware/software stack, the next meaningful UI-performance win is reducing unnecessary LVGL text/layout work, not pushing the WS175 display path toward risky full-frame PSRAM buffering.
- For storage writes, lock hold time matters more than raw flush frequency; moving NVS I/O out of the mutex is a better ROI optimization than simply stretching the timer further.
- On the OBD/UI side, page-switch correctness and performance are coupled: if swipe navigation does not immediately refresh demand state, the system wastes bandwidth polling data for no-longer-visible widgets.
- In the OBD task, merely "skipping send" is not enough. If skipped slots still consume the same delay budget, low-cardinality dashboards can feel much slower than they should.
- For this product shape, a unified horizontal settings IA gives better UX and safer round-screen layout than mixing vertical detail pages with return gestures.

## Verification

- `powershell -ExecutionPolicy Bypass -File .\tools\run_ui_platform_static_tests.ps1`
- `idf.py build`
- The static test bundle now includes the active-page refresh, settings hierarchy, dashboard-config fallback, and NVS-flush contention contracts, so later refactors have a higher chance of catching silent UX or performance regressions.
- Demand-driven workflow contracts now also cover swipe-driven active-page changes, not only programmatic page loads.
- OBD poll scheduling now has both:
  - a pure host-testable helper
  - a source-level contract ensuring the runtime jumps to demanded slots instead of linearly burning skipped delays

## Next time

- Consider page-type-aware refresh periods:
  - `G-force` pages may justify a faster cadence
  - menu/static pages can likely run slower
- Extract more dashboard-config rules into host-testable pure logic so later page-type expansion stays cheaper to verify.
- Keep the WS175 line-buffer strategy unless a dedicated hardware revalidation proves full-frame buffering is actually safer and faster on-device.
- Extend the same demand-driven pattern to `Oil Pressure`:
  - only sample ADS1115 while the active page actually uses `OIP`
  - keep room for future external subscribers, similar to the current `BKT` / RaceChrono gating model

## Follow-up 2026-07-03

### Why this polish round expanded

This work stopped being a pure visual cleanup. It turned into a combined UX, sensor-demand, storage-safety, and runtime-structure pass because the user feedback exposed cross-layer issues:

- settings and OBD interaction were not using the same mental model as the dashboard pages
- several modals had inconsistent semantics, including reboot-confirm behavior
- G-force pages still had unresolved direction/origin issues and weak use of round-screen area
- background polling and NVS flush behavior were still spending work outside actual UI demand

### High-value decisions merged from the temporary rationale snapshot

- `Settings` now owns the OBD protocol path; the old separate OBD protocol screen is legacy debt, not the target UX.
- Modal behavior should stay unified around the same shell-theme and confirm/cancel semantics.
- ZC6 gear should prefer real monitor/decode data, including `R`, rather than simulated-only behavior.
- Dashboard item availability is a vehicle-capability problem, not a one-off UI exception.
- Generated host-test `.o` files are build artifacts and should not be treated as meaningful source changes.

### Performance finding worth keeping

Home horizontal page switching was instrumented with `ui_home_perf`. The current evidence shows:

- the expensive part is not page-specific mount or refresh logic
- the active bottleneck is the horizontal tile/scroll path itself
- the same two-full-screen redraw pattern appears with `rot=0` and with `rot=180`

This means future performance work should focus on the home page-switch container model first, not on per-page content tweaks.
