#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <WiFi.h>
#include "MatrixDriver.h"
#include "secrets.h"

namespace {
constexpr uint16_t kWidth = 100;
constexpr uint16_t kHeight = 10;
constexpr float kDefaultSpeed = 24.0F;

WebSocketsClient socketClient;
SerialMatrixDriver matrix;
String currentText;
float currentSpeed = kDefaultSpeed;
uint32_t messageStartedAt = 0;
uint32_t lastFrameAt = 0;

void handleCommand(uint8_t* payload, size_t length) {
  JsonDocument document;
  const auto error = deserializeJson(document, payload, length);
  if (error) {
    Serial.printf("invalid command JSON: %s\\n", error.c_str());
    return;
  }

  const String type = document["type"] | "";
  if (type == "show") {
    currentText = String(document["payload"]["text"] | "");
    currentSpeed = document["payload"]["speed_pixels_per_second"] | kDefaultSpeed;
    messageStartedAt = millis();
  } else if (type == "clear") {
    currentText = "";
    matrix.clear();
    matrix.present();
  }
}

void onSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED: {
      JsonDocument hello;
      hello["type"] = "hello";
      hello["payload"]["device_id"] = LEDDY_DEVICE_ID;
      hello["payload"]["firmware_version"] = LEDDY_FIRMWARE_VERSION;
      hello["payload"]["capabilities"]["max_width"] = kWidth;
      hello["payload"]["capabilities"]["max_height"] = kHeight;
      hello["payload"]["capabilities"]["color_depth_bits"] = 1;
      hello["payload"]["capabilities"]["supports_brightness"] = true;
      String json;
      serializeJson(hello, json);
      socketClient.sendTXT(json);
      break;
    }
    case WStype_TEXT:
      handleCommand(payload, length);
      break;
    default:
      break;
  }
}

void renderTick() {
  const uint32_t now = millis();
  if (now - lastFrameAt < 33 || currentText.isEmpty()) return;
  lastFrameAt = now;
  const float elapsedSeconds = static_cast<float>(now - messageStartedAt) / 1000.0F;
  const int32_t contentWidth = static_cast<int32_t>(currentText.length() * 6U);
  const int32_t travel = contentWidth + kWidth;
  const int32_t offset = static_cast<int32_t>(elapsedSeconds * currentSpeed) % max(1L, static_cast<long>(travel)) - kWidth;
  matrix.drawTextFrame(currentText, offset);
  matrix.present();
}
}  // namespace

void setup() {
  Serial.begin(115200);
  matrix.begin(kWidth, kHeight);
  WiFi.begin(LEDDY_WIFI_SSID, LEDDY_WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(250);
  socketClient.begin(LEDDY_WS_HOST, LEDDY_WS_PORT, LEDDY_WS_PATH);
  socketClient.onEvent(onSocketEvent);
  socketClient.setReconnectInterval(2000);
}

void loop() {
  socketClient.loop();
  renderTick();
}
