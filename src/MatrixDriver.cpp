#include "MatrixDriver.h"

namespace {
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
  return "unknown";
}
}  // namespace

void SerialMatrixDriver::begin(uint16_t width, uint16_t height) {
  config_.width = width;
  config_.height = height;
}

void SerialMatrixDriver::configure(const MatrixConfig& config) {
  config_ = config;
}

void SerialMatrixDriver::clear() {
  text_ = "";
  offset_ = 0;
}

void SerialMatrixDriver::drawTextFrame(const String& text, int32_t offset) {
  text_ = text;
  offset_ = offset;
}

void SerialMatrixDriver::present() {
  Serial.printf(
      "[matrix %ux%u] brightness=%u serpentine=%s origin=%s offset=%ld text=%s\n",
      config_.width,
      config_.height,
      config_.brightness,
      config_.serpentine ? "true" : "false",
      originName(config_.origin),
      static_cast<long>(offset_),
      text_.c_str());
}
