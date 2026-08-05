#include <string.h>

#include "leddy_port.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_esp32_exposes_wireless_transports(void) {
    const leddy_capabilities_t capabilities =
        leddy_capabilities_for(LEDDY_PLATFORM_ESP32);

    TEST_ASSERT_EQUAL_STRING("esp32", leddy_platform_name(capabilities.platform));
    TEST_ASSERT_TRUE(
        leddy_transport_supported(&capabilities, LEDDY_TRANSPORT_WIFI)
    );
    TEST_ASSERT_TRUE(
        leddy_transport_supported(&capabilities, LEDDY_TRANSPORT_BLE)
    );
}

static void test_stm32_rejects_oversized_width(void) {
    const leddy_capabilities_t capabilities =
        leddy_capabilities_for(LEDDY_PLATFORM_STM32);
    const leddy_display_request_t request = {
        .width = 300,
        .height = 20,
        .brightness = 96,
        .bytes_per_pixel = 3,
    };
    leddy_frame_plan_t plan;

    TEST_ASSERT_EQUAL_INT(
        LEDDY_UNSUPPORTED_DIMENSIONS,
        leddy_plan_frame(&capabilities, &request, 128u * 1024u, &plan)
    );
}

static void test_frame_plan_accounts_for_row_buffer(void) {
    const leddy_capabilities_t capabilities =
        leddy_capabilities_for(LEDDY_PLATFORM_ESP32);
    const leddy_display_request_t request = {
        .width = 300,
        .height = 20,
        .brightness = 96,
        .bytes_per_pixel = 3,
    };
    leddy_frame_plan_t plan;

    TEST_ASSERT_EQUAL_INT(
        LEDDY_VALID,
        leddy_plan_frame(&capabilities, &request, 20u * 1024u, &plan)
    );
    TEST_ASSERT_EQUAL_UINT32(6000u, plan.pixel_count);
    TEST_ASSERT_EQUAL_UINT32(18000u, plan.framebuffer_bytes);
    TEST_ASSERT_EQUAL_UINT32(900u, plan.row_buffer_bytes);

    TEST_ASSERT_EQUAL_INT(
        LEDDY_INSUFFICIENT_MEMORY,
        leddy_plan_frame(&capabilities, &request, 18000u, &plan)
    );
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_esp32_exposes_wireless_transports);
    RUN_TEST(test_stm32_rejects_oversized_width);
    RUN_TEST(test_frame_plan_accounts_for_row_buffer);
    return UNITY_END();
}
