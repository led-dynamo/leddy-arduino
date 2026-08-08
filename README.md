# leddy-arduino

Embedded firmware workspace for Leddy-compatible LED controllers. The Arduino
framework target on ESP32 is the network-connected reference firmware; native
ESP-IDF and STM32Cube ports provide additional controller options behind the
same portable capability/frame-planning layer.

## Supported targets

| PlatformIO environment | Controller / framework | Purpose |
|---|---|---|
| `esp32dev` | ESP32 using Arduino | Wi-Fi/WebSocket reference firmware and matrix-driver boundary |
| `esp32s3-idf` | ESP32-S3 using native ESP-IDF | Native FreeRTOS port with Wi-Fi, BLE, USB-serial capabilities, and renderer hooks |
| `nucleo_f446re` | STM32F446RE using STM32Cube HAL | Deterministic matrix scanning with overridable board-clock, telemetry, and display hooks |
| `native` | Host compiler | Capability, memory-budget, direction, and repeat-lifecycle tests |

## ESP32 device protocol

The Arduino/ESP32 target consumes the same tagged JSON command/event contract as
`leddy-api-server.rs`, the Raspberry Pi agent, and the virtual E2E device.

Supported commands:

- `configure` — width, height, brightness, serpentine wiring, and pixel origin;
- `show` — arbitrary text beyond the physical board width, speed, left/right
  direction, and once/forever/count repeat mode;
- `clear` — immediately blank the display;
- `ping` — return a protocol `pong` event.

The firmware emits:

- `hello` with ESP32 platform metadata and USB-serial/Wi-Fi/BLE transports;
- `ack` after accepted show/clear/configure commands;
- periodic `telemetry` with uptime, free heap, Wi-Fi RSSI, and current message;
- `pong` for protocol pings;
- structured `error` events for malformed or unsupported commands.

Scrolling is calculated from elapsed time, so the firmware retains only the
message plus one display frame/driver state instead of materializing every
animation frame. The portable `leddy_playback_at` implementation is covered by
host-native tests for left/right entry positions and once/forever/count cycle
boundaries.

The product display range is 100–300 pixels wide and 5–20 pixels tall. The
ESP32 frame planner checks requested dimensions against both controller limits
and a bounded share of currently free heap before accepting configuration.

## Build and test

```sh
cp include/secrets.example.h include/secrets.h
# edit local development credentials; never commit this file

pio run -e esp32dev
pio run -e esp32s3-idf
pio run -e nucleo_f446re
pio test -e native
bash scripts/test-controller-extraction.sh
```

Upload by selecting the connected target, for example:

```sh
pio run -e esp32dev --target upload
pio run -e esp32s3-idf --target upload
pio run -e nucleo_f446re --target upload
```

## Standalone controller repository packaging

The native ESP32-S3 and STM32Cube ports are intentionally extraction-ready.
Until `led-dynamo/leddy-esp32` and `led-dynamo/leddy-stm32` exist, this
repository remains their canonical source.

Generate either complete repository tree without network access:

```sh
bash scripts/extract-controller-repo.sh esp32 /tmp/leddy-esp32
bash scripts/extract-controller-repo.sh stm32 /tmp/leddy-stm32
```

Each generated tree contains its controller entrypoint, the shared embedded C
contract layer, host-native tests, PlatformIO configuration, GitHub Actions CI,
governance files, and an `docs/ORIGIN.md` migration record. CI runs the
extraction smoke test so a source change cannot silently make either generated
repository incomplete.

After the GitHub repositories are created, publish the generated trees first,
update `leddy-monorepo` to reference them, and only then remove the duplicated
native port from this repository.

## Wi-Fi provisioning and credentials

The current development target uses compile-time values in `include/secrets.h`.
That file is ignored. This is suitable for lab hardware only; production units
should move credentials into encrypted/protected device storage and a local
provisioning flow before distribution. The runtime does not block forever on
initial Wi-Fi association: after a bounded initial wait it continues running and
retries Wi-Fi while the WebSocket client applies its own reconnect interval.

## OTA and recovery strategy

The current repository does not silently self-update. The intended production
ESP32 path is a signed image delivered to an ESP-IDF/Arduino OTA partition with
rollback to the previously bootable image if the new firmware does not reach a
healthy checkpoint. Firmware rollout must preserve a physical recovery path
(USB serial/programmer) and must never overwrite Wi-Fi/device identity secrets.
Until signed OTA and rollback are implemented and tested, releases are flashed
explicitly through PlatformIO.

A malformed command produces a structured error rather than rebooting the
controller. Network loss leaves the current local display state intact while
reconnect continues. A server `clear` remains the canonical emergency blanking
command after connectivity returns.

## Hardware driver boundary

`MatrixDriver` now receives explicit width, height, brightness, serpentine, and
origin configuration. `SerialMatrixDriver` remains the CI/lab baseline. A real
panel driver must implement those semantics without changing network protocol
code.

See [`docs/HARDWARE_SMOKE_TEST.md`](docs/HARDWARE_SMOKE_TEST.md) before wiring or
claiming a physical target as validated.

`include/secrets.h` must never be committed.
