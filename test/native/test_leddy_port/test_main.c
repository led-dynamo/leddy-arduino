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

static leddy_playback_request_t playback_request(void) {
    return (leddy_playback_request_t){
        .content_width = 30u,
        .display_width = 100u,
        .speed_pixels_per_second = 20.0f,
        .direction = LEDDY_SCROLL_LEFT,
        .repeat = LEDDY_REPEAT_FOREVER,
        .repeat_count = 0u,
    };
}

static void test_playback_matches_left_and_right_entry_offsets(void) {
    leddy_playback_request_t request = playback_request();
    leddy_playback_state_t state;

    TEST_ASSERT_EQUAL_INT(
        LEDDY_VALID,
        leddy_playback_at(&request, 0u, &state)
    );
    TEST_ASSERT_TRUE(state.active);
    TEST_ASSERT_EQUAL_INT32(-100, state.offset);
    TEST_ASSERT_EQUAL_UINT32(6500u, state.cycle_duration_ms);

    request.direction = LEDDY_SCROLL_RIGHT;
    TEST_ASSERT_EQUAL_INT(
        LEDDY_VALID,
        leddy_playback_at(&request, 0u, &state)
    );
    TEST_ASSERT_EQUAL_INT32(30, state.offset);
}

static void test_once_and_counted_playback_stop_at_cycle_boundaries(void) {
    leddy_playback_request_t request = playback_request();
    leddy_playback_state_t state;

    request.repeat = LEDDY_REPEAT_ONCE;
    TEST_ASSERT_EQUAL_INT(
        LEDDY_VALID,
        leddy_playback_at(&request, 6499u, &state)
    );
    TEST_ASSERT_TRUE(state.active);
    TEST_ASSERT_EQUAL_INT(
        LEDDY_VALID,
        leddy_playback_at(&request, 6500u, &state)
    );
    TEST_ASSERT_FALSE(state.active);

    request.repeat = LEDDY_REPEAT_COUNT;
    request.repeat_count = 2u;
    TEST_ASSERT_EQUAL_INT(
        LEDDY_VALID,
        leddy_playback_at(&request, 6500u, &state)
    );
    TEST_ASSERT_TRUE(state.active);
    TEST_ASSERT_EQUAL_INT(
        LEDDY_VALID,
        leddy_playback_at(&request, 13000u, &state)
    );
    TEST_ASSERT_FALSE(state.active);
}

static void test_playback_rejects_invalid_speed(void) {
    leddy_playback_request_t request = playback_request();
    leddy_playback_state_t state;

    request.speed_pixels_per_second = 0.0f;
    TEST_ASSERT_EQUAL_INT(
        LEDDY_INVALID_PLAYBACK,
        leddy_playback_at(&request, 0u, &state)
    );
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_esp32_exposes_wireless_transports);
    RUN_TEST(test_stm32_rejects_oversized_width);
    RUN_TEST(test_frame_plan_accounts_for_row_buffer);
    RUN_TEST(test_playback_matches_left_and_right_entry_offsets);
    RUN_TEST(test_once_and_counted_playback_stop_at_cycle_boundaries);
    RUN_TEST(test_playback_rejects_invalid_speed);
    return UNITY_END();
}
