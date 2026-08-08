#ifndef LEDDY_PORT_H
#define LEDDY_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LEDDY_PLATFORM_ARDUINO = 0,
    LEDDY_PLATFORM_RASPBERRY_PI = 1,
    LEDDY_PLATFORM_ESP32 = 2,
    LEDDY_PLATFORM_STM32 = 3,
} leddy_platform_t;

typedef enum {
    LEDDY_TRANSPORT_NONE = 0,
    LEDDY_TRANSPORT_USB_SERIAL = 1u << 0,
    LEDDY_TRANSPORT_WIFI = 1u << 1,
    LEDDY_TRANSPORT_BLE = 1u << 2,
    LEDDY_TRANSPORT_ETHERNET = 1u << 3,
} leddy_transport_t;

typedef struct {
    leddy_platform_t platform;
    uint16_t max_width;
    uint16_t max_height;
    uint8_t color_depth_bits;
    bool supports_brightness;
    uint32_t transport_mask;
} leddy_capabilities_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t brightness;
    uint8_t bytes_per_pixel;
} leddy_display_request_t;

typedef struct {
    size_t pixel_count;
    size_t framebuffer_bytes;
    size_t row_buffer_bytes;
} leddy_frame_plan_t;

typedef enum {
    LEDDY_SCROLL_LEFT = 0,
    LEDDY_SCROLL_RIGHT = 1,
} leddy_scroll_direction_t;

typedef enum {
    LEDDY_REPEAT_ONCE = 0,
    LEDDY_REPEAT_FOREVER = 1,
    LEDDY_REPEAT_COUNT = 2,
} leddy_repeat_mode_t;

typedef struct {
    size_t content_width;
    uint16_t display_width;
    float speed_pixels_per_second;
    leddy_scroll_direction_t direction;
    leddy_repeat_mode_t repeat;
    uint32_t repeat_count;
} leddy_playback_request_t;

typedef struct {
    bool active;
    int32_t offset;
    uint32_t cycle_duration_ms;
} leddy_playback_state_t;

typedef enum {
    LEDDY_VALID = 0,
    LEDDY_INVALID_DIMENSIONS,
    LEDDY_UNSUPPORTED_DIMENSIONS,
    LEDDY_INVALID_PIXEL_FORMAT,
    LEDDY_INSUFFICIENT_MEMORY,
    LEDDY_INVALID_PLAYBACK,
} leddy_validation_t;

const char *leddy_platform_name(leddy_platform_t platform);
leddy_capabilities_t leddy_capabilities_for(leddy_platform_t platform);
bool leddy_transport_supported(
    const leddy_capabilities_t *capabilities,
    leddy_transport_t transport
);
leddy_validation_t leddy_plan_frame(
    const leddy_capabilities_t *capabilities,
    const leddy_display_request_t *request,
    size_t memory_budget_bytes,
    leddy_frame_plan_t *plan
);
leddy_validation_t leddy_playback_at(
    const leddy_playback_request_t *request,
    uint32_t elapsed_ms,
    leddy_playback_state_t *state
);

#ifdef __cplusplus
}
#endif

#endif
