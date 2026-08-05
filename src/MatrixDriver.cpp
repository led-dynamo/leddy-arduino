#include "MatrixDriver.h"

void SerialMatrixDriver::begin(uint16_t width, uint16_t height) {
  width_ = width;
  height_ = height;
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
  Serial.printf("[matrix %ux%u] offset=%ld text=%s\\n", width_, height_, static_cast<long>(offset_), text_.c_str());
}
