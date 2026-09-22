# Stability-first DYA firmware

This branch is derived from the build-successful `dya-studio-stage2-macro-combo`
branch and prioritizes reliable wireless operation over aggressive power saving.

## Stability changes

- Removed the custom split-BLE connection-interval power manager from the
  firmware build. The central/peripheral link remains on ZMK's standard fixed
  15 ms split interval instead of changing after 5 / 15 / 30 seconds.
- Explicitly keeps the standard split BLE baseline:
  - interval: 12 (15 ms)
  - latency: 30
  - supervision timeout: 400
- Keeps `CONFIG_ZMK_BLE_EXPERIMENTAL_CONN=y` from the existing shield config.
- Gives BLE mouse reports a queue size of 40 for bursty pointing traffic.
- Separates remote trackball processing from remote mini-trackpad processing.
  The double-ball remote trackball now uses the same runtime pointer and Scroll
  layer processing as the local trackball.
- Pins all moving project revisions in `config/west.yml` to exact commits.

## Recommended deployed roles

Do not swap central/peripheral roles during normal use.

For the standard single-ball + mini-trackpad configuration, use:

- `torabo_tsuki_lp_left_central` on the left half
- `torabo_tsuki_lp_right_peripheral` on the right half

For double-ball, use:

- `torabo_tsuki_lp_double_ball_right_central` on the right half
- `torabo_tsuki_lp_double_ball_left_peripheral` on the left half

## Clean pairing procedure for stability testing

When first moving to this branch:

1. Flash `settings_reset` to both halves.
2. Power-cycle both halves.
3. Flash the selected central firmware to the chosen central half.
4. Flash the matching peripheral firmware to the other half.
5. Let the two halves establish their split bond.
6. Remove the old host Bluetooth entry for the keyboard.
7. Pair the central half to the host again.

Avoid changing central side after this reset/pairing process.

## What is intentionally unchanged

- DYA Studio
- Runtime pointing settings
- Runtime Macro / Combo
- Keymap and layer geometry
- Mouse / BT layer
- Scroll layer index 4
- Default runtime scroll scale 1/12
- +8 dBm radio TX setting

The +8 dBm TX level is left unchanged for this first stability pass so that RF
power and split-link behavior are not changed at the same time. If instability
remains after this firmware, the next controlled test should compare +8 dBm
against a lower TX level while watching for power-supply related resets.
