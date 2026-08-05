#include "leddy_port.h"

#include <stdint.h>

const char *leddy_platform_name(leddy_platform_t platform) {
    switch (platform) {
        case LEDDY_PLATFORM_ARDUINO:
            return "arduino";
        case LEDDY_PLATFORM_RASPBERRY_PI:
            return "raspberry_pi";
        case LEDDY_PLATFORM_ESP32:
            return "esp32";
        case LEDDY_PLATFORM_STM32:
            return "stm32";
        default:
            return "unknown";
    }
}

leddy_capabilities_t leddy_capabilities_for(leddy_platform_t platform) {
    switch (platform) {
        case LEDDY_PLATFORM_ESP32:
            return (leddy_capabilities_t){
                .platform = platform,
                .max_width = 512,
                .max_height = 64,
                .color_depth_bits = 24,
                .supports_brightness = true,
                .transport_mask = LEDDY_TRANSPORT_USB_SERIAL |
                                  LEDDY_TRANSPORT_WIFI |
                                  LEDDY_TRANSPORT_BLE,
            };
        case LEDDY_PLATFORM_STM32:
            return (leddy_capabilities_t){
                .platform = platform,
                .max_width = 256,
                .max_height = 64,
                .color_depth_bits = 24,
                .supports_brightness = true,
                .transport_mask = LEDDY_TRANSPORT_USB_SERIAL |
                                  LEDDY_TRANSPORT_ETHERNET,
            };
        case LEDDY_PLATFORM_RASPBERRY_PI:
            return (leddy_capabilities_t){
                .platform = platform,
                .max_width = 4096,
                .max_height = 512,
                .color_depth_bits = 24,
                .supports_brightness = true,
                .transport_mask = LEDDY_TRANSPORT_USB_SERIAL |
                                  LEDDY_TRANSPORT_WIFI |
                                  LEDDY_TRANSPORT_ETHERNET,
            };
        case LEDDY_PLATFORM_ARDUINO:
        default:
            return (leddy_capabilities_t){
                .platform = LEDDY_PLATFORM_ARDUINO,
                .max_width = 128,
                .max_height = 32,
                .color_depth_bits = 24,
                .supports_brightness = true,
                .transport_mask = LEDDY_TRANSPORT_USB_SERIAL,
            };
    }
}

bool leddy_transport_supported(
    const leddy_capabilities_t *capabilities,
    leddy_transport_t transport
) {
    if (capabilities == NULL || transport == LEDDY_TRANSPORT_NONE) {
        return false;
    }
    return (capabilities->transport_mask & (uint32_t)transport) != 0u;
}

leddy_validation_t leddy_plan_frame(
    const leddy_capabilities_t *capabilities,
    const leddy_display_request_t *request,
    size_t memory_budget_bytes,
    leddy_frame_plan_t *plan
) {
    size_t pixel_count;
    size_t framebuffer_bytes;
    size_t row_buffer_bytes;

    if (capabilities == NULL || request == NULL || plan == NULL ||
        request->width == 0u || request->height == 0u) {
        return LEDDY_INVALID_DIMENSIONS;
    }
    if (request->width > capabilities->max_width ||
        request->height > capabilities->max_height) {
        return LEDDY_UNSUPPORTED_DIMENSIONS;
    }
    if (request->bytes_per_pixel == 0u || request->bytes_per_pixel > 4u) {
        return LEDDY_INVALID_PIXEL_FORMAT;
    }

    pixel_count = (size_t)request->width * (size_t)request->height;
    if (pixel_count > SIZE_MAX / (size_t)request->bytes_per_pixel) {
        return LEDDY_INSUFFICIENT_MEMORY;
    }

    framebuffer_bytes = pixel_count * (size_t)request->bytes_per_pixel;
    row_buffer_bytes = (size_t)request->width *
                       (size_t)request->bytes_per_pixel;

    if (framebuffer_bytes > memory_budget_bytes ||
        row_buffer_bytes > memory_budget_bytes - framebuffer_bytes) {
        return LEDDY_INSUFFICIENT_MEMORY;
    }

    plan->pixel_count = pixel_count;
    plan->framebuffer_bytes = framebuffer_bytes;
    plan->row_buffer_bytes = row_buffer_bytes;
    return LEDDY_VALID;
}
