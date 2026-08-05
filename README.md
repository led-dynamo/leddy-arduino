# leddy-arduino

Embedded firmware workspace for Leddy-compatible LED controllers. The original
Arduino/ESP32 implementation remains the default, and the repository now also
contains native ports that can be extracted into standalone repositories without
changing their source layout.

## Supported targets

| PlatformIO environment | Controller / framework | Purpose |
|---|---|---|
| `esp32dev` | ESP32 using Arduino | Existing Wi-Fi/WebSocket firmware and matrix-driver boundary |
| `esp32s3-idf` | ESP32-S3 using native ESP-IDF | Native FreeRTOS port with Wi-Fi, BLE, USB-serial capabilities, and renderer hooks |
| `nucleo_f446re` | STM32F446RE using STM32Cube HAL | Deterministic matrix scanning with overridable board-clock, telemetry, and display hooks |
| `native` | Host compiler | Capability, transport, display-limit, and framebuffer-memory tests |

The common C layer in `src/ports/common` prevents platform-specific firmware
from accepting a display configuration that exceeds controller dimensions or
its framebuffer memory budget. Native ports live under `src/ports/esp32-idf`
and `src/ports/stm32cube`.

## Build and test

```sh
cp include/secrets.example.h include/secrets.h
# edit credentials only for the Arduino/WebSocket target

pio run -e esp32dev
pio run -e esp32s3-idf
pio run -e nucleo_f446re
pio test -e native
```

Upload by selecting the connected target, for example:

```sh
pio run -e esp32s3-idf --target upload
pio run -e nucleo_f446re --target upload
```

`include/secrets.h` is ignored and must never be committed. Hardware-specific
LED libraries stay behind `MatrixDriver` for Arduino or the weak
`leddy_matrix_present` hook for the native ports.
