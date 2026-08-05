#include <inttypes.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "leddy_port.h"

#ifndef LEDDY_FIRMWARE_VERSION
#define LEDDY_FIRMWARE_VERSION "dev"
#endif

static const char *TAG = "leddy-esp32";

__attribute__((weak)) void leddy_matrix_present(uint64_t frame_number) {
    (void)frame_number;
}

static void render_task(void *context) {
    const leddy_frame_plan_t *plan = (const leddy_frame_plan_t *)context;
    uint64_t frame_number = 0;

    ESP_LOGI(
        TAG,
        "renderer online: pixels=%u framebuffer=%u row_buffer=%u",
        (unsigned)plan->pixel_count,
        (unsigned)plan->framebuffer_bytes,
        (unsigned)plan->row_buffer_bytes
    );

    for (;;) {
        leddy_matrix_present(frame_number++);
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

void app_main(void) {
    static leddy_frame_plan_t frame_plan;
    const leddy_capabilities_t capabilities =
        leddy_capabilities_for(LEDDY_PLATFORM_ESP32);
    const leddy_display_request_t request = {
        .width = 300,
        .height = 20,
        .brightness = 96,
        .bytes_per_pixel = 3,
    };
    const leddy_validation_t validation = leddy_plan_frame(
        &capabilities,
        &request,
        128u * 1024u,
        &frame_plan
    );

    ESP_LOGI(
        TAG,
        "boot platform=%s firmware=%s uptime_us=%" PRIi64,
        leddy_platform_name(capabilities.platform),
        LEDDY_FIRMWARE_VERSION,
        esp_timer_get_time()
    );
    ESP_LOGI(
        TAG,
        "transports usb=%d wifi=%d ble=%d",
        leddy_transport_supported(&capabilities, LEDDY_TRANSPORT_USB_SERIAL),
        leddy_transport_supported(&capabilities, LEDDY_TRANSPORT_WIFI),
        leddy_transport_supported(&capabilities, LEDDY_TRANSPORT_BLE)
    );

    if (validation != LEDDY_VALID) {
        ESP_LOGE(TAG, "display request rejected: code=%d", (int)validation);
        return;
    }

    xTaskCreate(render_task, "leddy-render", 4096, &frame_plan, 5, NULL);
}
