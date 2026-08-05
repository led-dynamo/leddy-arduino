#pragma once

#include <Arduino.h>

class MatrixDriver {
 public:
  virtual ~MatrixDriver() = default;
  virtual void begin(uint16_t width, uint16_t height) = 0;
  virtual void clear() = 0;
  virtual void drawTextFrame(const String& text, int32_t offset) = 0;
  virtual void present() = 0;
};

class SerialMatrixDriver final : public MatrixDriver {
 public:
  void begin(uint16_t width, uint16_t height) override;
  void clear() override;
  void drawTextFrame(const String& text, int32_t offset) override;
  void present() override;
 private:
  uint16_t width_ = 0;
  uint16_t height_ = 0;
  String text_;
  int32_t offset_ = 0;
};
