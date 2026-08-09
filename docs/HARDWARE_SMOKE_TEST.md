# Leddy embedded hardware smoke test

Use this checklist after host/native CI is green and before declaring an ESP32
or other embedded controller ready for a physical Leddy sign.

## Preconditions

- Record the exact board revision, panel/LED model, supply voltage, logic-level
  requirements, and total current budget.
- Use an externally fused supply sized from the panel/LED datasheet. Do not
  power a large matrix from the controller board's regulator or USB port.
- Connect controller and LED supply grounds and use the required level
  shifter/buffer for the selected data interface.
- Start at the minimum supported fixture (100×5) and a conservative brightness.
- Flash a CI-green commit and retain USB/programmer access for recovery.

## Network and protocol

1. Boot with the API unavailable. Confirm the controller remains responsive and
   retries rather than blocking or reboot-looping.
2. Start the API and confirm one `hello` device record appears with platform
   `esp32` and expected transports.
3. Send protocol `ping` and confirm `pong` with the same nonce.
4. Send malformed JSON and an unsupported command; confirm structured errors and
   no reboot.
5. Interrupt Wi-Fi, restore it, and confirm WebSocket reconnection and desired
   state replay.

## Display behavior

For each supported origin and both row-major/serpentine wiring modes:

1. Configure the fixture and confirm the command is acknowledged.
2. Show a message much wider than the physical display and verify continuous
   left scrolling.
3. Repeat with right scrolling.
4. Verify `once` stops after one traversal, `count: 2` stops after two, and
   `forever` remains active.
5. Change brightness at runtime and verify the physical driver applies it without
   corrupting layout.
6. Send `clear` and verify immediate blanking.
7. Power-cycle after a safe blank and repeat startup/reconnect checks.

## Telemetry and memory

- Confirm periodic telemetry contains uptime, non-zero free heap, Wi-Fi RSSI,
  and the active message ID.
- Run a 300×20 fixture with a long message for at least 30 minutes and record
  minimum free heap. There should be no monotonic heap loss.
- Verify rejected configurations do not change the active matrix state.

## Failure and shutdown

- Remove network connectivity while displaying and ensure the renderer remains
  deterministic locally.
- Restore connectivity and issue emergency `clear`.
- Blank the display before removing matrix power.
- Record brownout, watchdog, reset reason, thermal, or signal-integrity failures
  together with the exact power and wiring topology.

Hardware validation evidence should include the commit SHA, controller/panel
models, configuration, power budget, photos/wiring diagram, serial log, and API
telemetry sample.
