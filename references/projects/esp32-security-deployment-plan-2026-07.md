## Context

- Product direction may move from hobby firmware to a commercial ESP32-S3 based device.
- Current hardware path uses purchased ESP32 finished development boards rather than a custom baseboard.
- Main concern: if the board remains in default development state, others can potentially read back firmware and clone the product.

## Accepted security direction

- Preferred production baseline is "Plan B":
  - Flash Encryption
  - Secure Boot V2
  - production/release mode
  - JTAG disabled
  - UART ROM download either disabled or reduced to secure mode
  - OTA remains available for signed images
- "Plan C" is also acceptable later if the product needs a tighter lockdown:
  - build on top of Plan B
  - stronger production key isolation
  - permanent download-path shutdown where service model allows it
  - optional NVS encryption / cloud identity hardening

## Practical impact

- Do not treat commercial security as a normal "flash my bin" workflow.
- Separate:
  - development boards
  - security-validation boards
  - production boards
- Avoid testing irreversible eFuse settings on the only convenient daily-use dev board.
- Best default recommendation for this repo:
  - prototype and feature work on normal dev boards
  - validate Secure Boot V2 + Flash Encryption on sacrificial boards
  - move to production configuration only after OTA / recovery / signing flow is proven

## Irreversibility notes

- The dangerous parts are not the sdkconfig toggles themselves, but the eFuse-backed security state they eventually burn into the chip.
- Once production security eFuses are burned, common irreversible effects can include:
  - flash becomes encrypted and no longer readable as plain firmware
  - secure boot requires signed images
  - JTAG may be permanently disabled
  - UART ROM download may be permanently restricted or disabled
- Therefore never use the only bring-up board as the first irreversible security test target.

## Follow-up

- If implementing this plan in-repo, prepare two explicit configurations:
  - security-validation config
  - production config
- Next useful task: map Plan B to exact ESP-IDF menuconfig items and a board-safe rollout procedure for this repository.

## Decision update

- User accepts both Plan B and Plan C as viable long-term directions.
- Additional confirmed constraint:
  - irreversible security validation must not be performed first on the only convenient daily-use development board
- Working rule going forward:
  - normal feature work stays on ordinary dev boards
  - any first-time Secure Boot / Flash Encryption / eFuse validation happens on sacrificial validation boards
  - production lockdown is a later, separate step after the recovery and OTA path is proven
