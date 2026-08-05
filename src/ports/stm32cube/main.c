#include <stdint.h>

#include "leddy_port.h"
#include "stm32f4xx_hal.h"

#ifndef LEDDY_FIRMWARE_VERSION
#define LEDDY_FIRMWARE_VERSION "dev"
#endif

static volatile leddy_validation_t g_boot_status = LEDDY_INVALID_DIMENSIONS;
static volatile uint32_t g_frame_number = 0u;

__attribute__((weak)) void leddy_board_clock_configure(void) {
    /* Board projects may override this with their generated clock tree. */
}

__attribute__((weak)) void leddy_matrix_present(uint32_t frame_number) {
    (void)frame_number;
}

__attribute__((weak)) void leddy_publish_boot_state(
    const char *platform,
    const char *firmware_version,
    leddy_validation_t status
) {
    (void)platform;
    (void)firmware_version;
    (void)status;
}

int main(void) {
    leddy_frame_plan_t frame_plan;
    const leddy_capabilities_t capabilities =
        leddy_capabilities_for(LEDDY_PLATFORM_STM32);
    const leddy_display_request_t request = {
        .width = 192,
        .height = 16,
        .brightness = 96,
        .bytes_per_pixel = 3,
    };

    HAL_Init();
    leddy_board_clock_configure();

    g_boot_status = leddy_plan_frame(
        &capabilities,
        &request,
        64u * 1024u,
        &frame_plan
    );
    leddy_publish_boot_state(
        leddy_platform_name(capabilities.platform),
        LEDDY_FIRMWARE_VERSION,
        g_boot_status
    );

    if (g_boot_status != LEDDY_VALID) {
        for (;;) {
            HAL_Delay(250u);
        }
    }

    for (;;) {
        leddy_matrix_present(g_frame_number++);
        HAL_Delay(16u);
    }
}
