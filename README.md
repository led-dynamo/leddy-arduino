# leddy-arduino

Arduino/ESP32 firmware boundary for Wi-Fi provisioning, WebSocket commands,
scroll timing, and matrix-driver integration. The first target is `esp32dev`
because it provides network connectivity and enough memory for configurable
matrices. Hardware-specific LED libraries are isolated behind `MatrixDriver`.

```sh
cp include/secrets.example.h include/secrets.h
# edit credentials, then:
pio run
pio run --target upload
```

`include/secrets.h` is ignored and must never be committed.
