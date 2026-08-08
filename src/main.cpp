#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <WiFi.h>
#include <math.h>

#include "MatrixDriver.h"
#include "leddy_port.h"
#include "secrets.h"

namespace {
constexpr uint16_t kMinWidth = 100;
constexpr uint16_t kMaxWidth = 300;
constexpr uint16_t kMinHeight = 5;
constexpr uint16_t kMaxHeight = 20;
constexpr float kDefaultSpeed = 24.0F;
constexpr size_t kMaxMessageCharacters = 16384U;
constexpr uint32_t kFrameIntervalMs = 33U;
constexpr uint32_t kTelemetryIntervalMs = 5000U;
constexpr uint32_t kWifiRetryIntervalMs = 5000U;
constexpr uint32_t kInitialWifiWaitMs = 30000U;

WebSocketsClient socketClient;
SerialMatrixDriver matrix;
MatrixConfig matrixConfig;
String currentText;
String currentMessageId;
leddy_playback_request_t playback{};
bool hasMessage = false;
bool socketConnected = false;
uint32_t messageStartedAt = 0;
uint32_t lastFrameAt = 0;
uint32_t lastTelemetryAt = 0;
uint32_t lastWifiRetryAt = 0;

size_t utf8CharacterCount(const String& text) {
  size_t count = 0;
  for (size_t index = 0; index < text.length(); ++index) {
    const uint8_t byte = static_cast<uint8_t>(text[index]);
    if ((byte & 0xC0U) != 0x80U) ++count;
  }
  return count;
}

const char* originName(MatrixPixelOrigin origin) {
  switch (origin) {
    case MatrixPixelOrigin::TopLeft:
      return "top_left";
    case MatrixPixelOrigin::TopRight:
      return "top_right";
    case MatrixPixelOrigin::BottomLeft:
      return "bottom_left";
    case MatrixPixelOrigin::BottomRight:
      return "bottom_right";
  }
  return "top_left";
}

bool parseOrigin(const char* value, MatrixPixelOrigin* origin) {
  if (value == nullptr || origin == nullptr) return false;
  if (strcmp(value, "top_left") == 0) {
    *origin = MatrixPixelOrigin::TopLeft;
  } else if (strcmp(value, "top_right") == 0) {
    *origin = MatrixPixelOrigin::TopRight;
  } else if (strcmp(value, "bottom_left") == 0) {
    *origin = MatrixPixelOrigin::BottomLeft;
  } else if (strcmp(value, "bottom_right") == 0) {
    *origin = MatrixPixelOrigin::BottomRight;
  } else {
    return false;
  }
  return true;
}

void sendDocument(JsonDocument& document) {
  if (!socketConnected) return;
  String json;
  serializeJson(document, json);
  socketClient.sendTXT(json);
}

void sendAck(const String& commandId) {
  JsonDocument document;
  document["type"] = "ack";
  document["payload"]["command_id"] = commandId;
  sendDocument(document);
}

void sendError(const char* code, const String& message) {
  JsonDocument document;
  document["type"] = "error";
  document["payload"]["code"] = code;
  document["payload"]["message"] = message;
  sendDocument(document);
}

void sendPong(const String& nonce) {
  JsonDocument document;
  document["type"] = "pong";
  document["payload"]["nonce"] = nonce;
  sendDocument(document);
}

void sendTelemetry() {
  if (!socketConnected) return;
  JsonDocument document;
  document["type"] = "telemetry";
  document["payload"]["device_id"] = LEDDY_DEVICE_ID;
  document["payload"]["uptime_seconds"] = millis() / 1000U;
  document["payload"]["free_memory_bytes"] = ESP.getFreeHeap();
  document["payload"]["temperature_celsius"] = nullptr;
  if (WiFi.status() == WL_CONNECTED) {
    document["payload"]["wifi_rssi_dbm"] = WiFi.RSSI();
  } else {
    document["payload"]["wifi_rssi_dbm"] = nullptr;
  }
  if (hasMessage) {
    document["payload"]["current_message_id"] = currentMessageId;
  } else {
    document["payload"]["current_message_id"] = nullptr;
  }
  sendDocument(document);
}

void sendHello() {
  JsonDocument hello;
  hello["type"] = "hello";
  hello["payload"]["device_id"] = LEDDY_DEVICE_ID;
  hello["payload"]["firmware_version"] = LEDDY_FIRMWARE_VERSION;
  hello["payload"]["capabilities"]["max_width"] = kMaxWidth;
  hello["payload"]["capabilities"]["max_height"] = kMaxHeight;
  hello["payload"]["capabilities"]["color_depth_bits"] = 24;
  hello["payload"]["capabilities"]["supports_brightness"] = true;
  hello["payload"]["capabilities"]["platform"] = "esp32";
  JsonArray transports = hello["payload"]["capabilities"]["transports"].to<JsonArray>();
  transports.add("usb_serial");
  transports.add("wifi");
  transports.add("ble");
  sendDocument(hello);
}

bool validateMatrixConfig(const MatrixConfig& config, String* reason) {
  if (config.width < kMinWidth || config.width > kMaxWidth ||
      config.height < kMinHeight || config.height > kMaxHeight) {
    if (reason != nullptr) {
      *reason = "matrix dimensions are outside the supported 100..300 x 5..20 range";
    }
    return false;
  }

  const leddy_capabilities_t capabilities = leddy_capabilities_for(LEDDY_PLATFORM_ESP32);
  const leddy_display_request_t request = {
      .width = config.width,
      .height = config.height,
      .brightness = config.brightness,
      .bytes_per_pixel = 1,
  };
  leddy_frame_plan_t plan;
  const size_t memoryBudget = static_cast<size_t>(ESP.getFreeHeap()) / 2U;
  if (leddy_plan_frame(&capabilities, &request, memoryBudget, &plan) != LEDDY_VALID) {
    if (reason != nullptr) *reason = "matrix configuration exceeds the ESP32 frame budget";
    return false;
  }
  return true;
}

bool parseRepeat(JsonVariantConst repeat, leddy_repeat_mode_t* mode, uint32_t* count) {
  if (mode == nullptr || count == nullptr) return false;
  const char* name = repeat.as<const char*>();
  if (name != nullptr) {
    if (strcmp(name, "once") == 0) {
      *mode = LEDDY_REPEAT_ONCE;
      *count = 1U;
      return true;
    }
    if (strcmp(name, "forever") == 0) {
      *mode = LEDDY_REPEAT_FOREVER;
      *count = 0U;
      return true;
    }
    return false;
  }

  const uint32_t requestedCount = repeat["count"] | 0U;
  if (requestedCount == 0U) return false;
  *mode = LEDDY_REPEAT_COUNT;
  *count = requestedCount;
  return true;
}

void clearDisplay(bool acknowledge) {
  hasMessage = false;
  currentText = "";
  currentMessageId = "";
  matrix.clear();
  matrix.present();
  if (acknowledge) sendAck("clear");
}

void handleShow(JsonObjectConst payload) {
  const String id = String(payload["id"] | "");
  const String text = String(payload["text"] | "");
  const float speed = payload["speed_pixels_per_second"] | kDefaultSpeed;
  const char* direction = payload["direction"] | "left";

  if (id.isEmpty()) {
    sendError("invalid_message_id", "message id must not be empty");
    return;
  }
  if (text.isEmpty()) {
    sendError("invalid_message", "message text must not be empty");
    return;
  }
  const size_t characterCount = utf8CharacterCount(text);
  if (characterCount == 0U || characterCount > kMaxMessageCharacters) {
    sendError("message_too_long", "message exceeds the firmware character limit");
    return;
  }
  if (!isfinite(speed) || speed <= 0.0F) {
    sendError("invalid_speed", "scroll speed must be a positive finite number");
    return;
  }

  leddy_scroll_direction_t scrollDirection;
  if (strcmp(direction, "left") == 0) {
    scrollDirection = LEDDY_SCROLL_LEFT;
  } else if (strcmp(direction, "right") == 0) {
    scrollDirection = LEDDY_SCROLL_RIGHT;
  } else {
    sendError("invalid_direction", "scroll direction must be left or right");
    return;
  }

  leddy_repeat_mode_t repeatMode;
  uint32_t repeatCount;
  if (!parseRepeat(payload["repeat"], &repeatMode, &repeatCount)) {
    sendError("invalid_repeat", "repeat must be once, forever, or a positive count");
    return;
  }

  playback.content_width = characterCount * 6U - 1U;
  playback.display_width = matrixConfig.width;
  playback.speed_pixels_per_second = speed;
  playback.direction = scrollDirection;
  playback.repeat = repeatMode;
  playback.repeat_count = repeatCount;
  leddy_playback_state_t state;
  if (leddy_playback_at(&playback, 0U, &state) != LEDDY_VALID) {
    sendError("invalid_playback", "message cannot be scheduled on this display");
    return;
  }

  currentText = text;
  currentMessageId = id;
  messageStartedAt = millis();
  hasMessage = true;
  sendAck(id);
  sendTelemetry();
}

void handleConfigure(JsonObjectConst payload) {
  MatrixConfig next = matrixConfig;
  next.width = payload["width"] | next.width;
  next.height = payload["height"] | next.height;
  next.brightness = payload["brightness"] | next.brightness;
  next.serpentine = payload["serpentine"] | next.serpentine;

  const char* origin = payload["origin"] | originName(next.origin);
  if (!parseOrigin(origin, &next.origin)) {
    sendError("invalid_origin", "unsupported pixel origin");
    return;
  }

  String reason;
  if (!validateMatrixConfig(next, &reason)) {
    sendError("invalid_configuration", reason);
    return;
  }

  matrixConfig = next;
  matrix.begin(matrixConfig.width, matrixConfig.height);
  matrix.configure(matrixConfig);
  if (hasMessage) {
    playback.display_width = matrixConfig.width;
    messageStartedAt = millis();
  }
  matrix.clear();
  matrix.present();
  sendAck("configure");
  sendTelemetry();
}

void handleCommand(uint8_t* payload, size_t length) {
  JsonDocument document;
  const auto error = deserializeJson(document, payload, length);
  if (error) {
    sendError("invalid_json", String("invalid command JSON: ") + error.c_str());
    return;
  }

  const String type = document["type"] | "";
  const JsonObjectConst commandPayload = document["payload"].as<JsonObjectConst>();
  if (type == "show") {
    handleShow(commandPayload);
  } else if (type == "clear") {
    clearDisplay(true);
    sendTelemetry();
  } else if (type == "configure") {
    handleConfigure(commandPayload);
  } else if (type == "ping") {
    const String nonce = String(commandPayload["nonce"] | "");
    if (nonce.isEmpty()) {
      sendError("invalid_ping", "ping nonce must not be empty");
    } else {
      sendPong(nonce);
    }
  } else {
    sendError("unsupported_command", String("unsupported command type: ") + type);
  }
}

void onSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      socketConnected = true;
      sendHello();
      sendTelemetry();
      break;
    case WStype_DISCONNECTED:
      socketConnected = false;
      break;
    case WStype_TEXT:
      handleCommand(payload, length);
      break;
    default:
      break;
  }
}

void renderTick() {
  const uint32_t now = millis();
  if (!hasMessage || now - lastFrameAt < kFrameIntervalMs) return;
  lastFrameAt = now;

  leddy_playback_state_t state;
  if (leddy_playback_at(&playback, now - messageStartedAt, &state) != LEDDY_VALID) {
    sendError("invalid_playback", "active message playback became invalid");
    clearDisplay(false);
    return;
  }
  if (!state.active) {
    clearDisplay(false);
    sendTelemetry();
    return;
  }

  matrix.drawTextFrame(currentText, state.offset);
  matrix.present();
}

void telemetryTick() {
  const uint32_t now = millis();
  if (!socketConnected || now - lastTelemetryAt < kTelemetryIntervalMs) return;
  lastTelemetryAt = now;
  sendTelemetry();
}

void wifiTick() {
  const uint32_t now = millis();
  if (WiFi.status() == WL_CONNECTED || now - lastWifiRetryAt < kWifiRetryIntervalMs) return;
  lastWifiRetryAt = now;
  WiFi.reconnect();
}
}  // namespace

void setup() {
  Serial.begin(115200);
  matrix.begin(matrixConfig.width, matrixConfig.height);
  matrix.configure(matrixConfig);

  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(LEDDY_WIFI_SSID, LEDDY_WIFI_PASSWORD);
  const uint32_t wifiStartedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartedAt < kInitialWifiWaitMs) {
    delay(250);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi not connected after initial wait; continuing with bounded reconnect attempts");
  }

  socketClient.begin(LEDDY_WS_HOST, LEDDY_WS_PORT, LEDDY_WS_PATH);
  socketClient.onEvent(onSocketEvent);
  socketClient.setReconnectInterval(2000);
}

void loop() {
  wifiTick();
  socketClient.loop();
  renderTick();
  telemetryTick();
}
