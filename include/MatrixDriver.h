#pragma once

#include <Arduino.h>

enum class MatrixPixelOrigin : uint8_t {
  TopLeft = 0,
  TopRight = 1,
  BottomLeft = 2,
  BottomRight = 3,
};

struct MatrixConfig {
  uint16_t width = 100;
  uint16_t height = 10;
  uint8_t brightness = 96;
  bool serpentine = true;
  MatrixPixelOrigin origin = MatrixPixelOrigin::TopLeft;
};

class MatrixDriver {
 public:
  virtual ~MatrixDriver() = default;
  virtual void begin(uint16_t width, uint16_t height) = 0;
  virtual void configure(const MatrixConfig& config) = 0;
  virtual void clear() = 0;
  virtual void drawTextFrame(const String& text, int32_t offset) = 0;
  virtual void present() = 0;
};

class SerialMatrixDriver final : public MatrixDriver {
 public:
  void begin(uint16_t width, uint16_t height) override;
  void configure(const MatrixConfig& config) override;
  void clear() override;
  void drawTextFrame(const String& text, int32_t offset) override;
  void present() override;

 private:
  MatrixConfig config_;
  String text_;
  int32_t offset_ = 0;
};
