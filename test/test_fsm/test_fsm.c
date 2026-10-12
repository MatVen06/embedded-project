#include <unity.h>
#include "recruiting/fsm/fsm.h"
#include "recruiting/telemetry/telemetry.h"
#include "recruiting/cli/cli.h"
#include "recruiting/button/button.h"

static uint32_t mock_tick = 0;
static int mock_pin13_val = 1;

uint32_t HAL_GetTick(void)
{
    return mock_tick;
}

int HAL_GPIO_ReadPin(void *port, uint16_t pin)
{
    (void)port;
    if (pin == GPIO_PIN_13)
    {
        return mock_pin13_val;
    }
    return 1;
}

void setUp(void)
{
    mock_tick = 0;
    mock_pin13_val = 1;
    button_test_reset();
    cli_test_reset();
    telemetry_filter_reset();
    fsm_test_reset();
}

void tearDown(void)
{
}

void test_fsm_initial_state_is_post(void)
{
    TEST_ASSERT_EQUAL(STATE_POST, fsm_test_get_state());
}

void test_fsm_post_settling_delay_below_100ms(void)
{
    mock_tick = 50;
    telemetry_test_set_raw_adc(2000, 2000);
    fsm_app_update();

    TEST_ASSERT_EQUAL(STATE_POST, fsm_test_get_state());
}

void test_fsm_post_success_transitions_to_waiting(void)
{
    mock_tick = 150;
    telemetry_test_set_raw_adc(2000, 2000);
    fsm_app_update();

    TEST_ASSERT_EQUAL(STATE_WAITING, fsm_test_get_state());
}

void test_fsm_post_pot_out_of_bounds_transitions_to_fatal(void)
{
    mock_tick = 150;
    telemetry_test_set_raw_adc(4090, 2000);
    fsm_app_update();

    TEST_ASSERT_EQUAL(STATE_FATAL, fsm_test_get_state());
    TEST_ASSERT_NOT_NULL(fsm_test_get_context()->error_description);
}

void test_fsm_post_ntc_out_of_bounds_transitions_to_fatal(void)
{
    mock_tick = 150;
    telemetry_test_set_raw_adc(2000, 50);
    fsm_app_update();

    TEST_ASSERT_EQUAL(STATE_FATAL, fsm_test_get_state());
    TEST_ASSERT_NOT_NULL(fsm_test_get_context()->error_description);
}

void test_fsm_waiting_to_listening_on_button_press(void)
{
    fsm_test_set_state(STATE_WAITING);
    button_test_trigger_press();
    fsm_app_update();

    TEST_ASSERT_EQUAL(STATE_LISTENING, fsm_test_get_state());
}

void test_fsm_waiting_to_listening_on_cli_run(void)
{
    fsm_test_set_state(STATE_WAITING);
    cli_test_inject_string("run\r");
    fsm_app_update();

    TEST_ASSERT_EQUAL(STATE_LISTENING, fsm_test_get_state());
}

void test_fsm_listening_to_paused_on_button_press(void)
{
    fsm_test_set_state(STATE_LISTENING);
    telemetry_test_set_raw_adc(2000, 2000);
    button_test_trigger_press();
    fsm_app_update();

    TEST_ASSERT_EQUAL(STATE_PAUSED, fsm_test_get_state());
}

void test_fsm_paused_to_listening_on_cli_run(void)
{
    fsm_test_set_state(STATE_PAUSED);
    cli_test_inject_string("run\n");
    fsm_app_update();

    TEST_ASSERT_EQUAL(STATE_LISTENING, fsm_test_get_state());
}

void test_fsm_listening_to_warning_after_5s_continuous_diff(void)
{
    fsm_test_set_state(STATE_LISTENING);
    telemetry_test_set_raw_adc(1000, 2500);

    mock_tick = 1000;
    fsm_app_update();
    TEST_ASSERT_EQUAL(STATE_LISTENING, fsm_test_get_state());

    mock_tick = 4000;
    fsm_app_update();
    TEST_ASSERT_EQUAL(STATE_LISTENING, fsm_test_get_state());

    mock_tick = 6000;
    fsm_app_update();
    TEST_ASSERT_EQUAL(STATE_WARNING, fsm_test_get_state());
}

void test_fsm_listening_diff_drop_resets_warning_timer(void)
{
    fsm_test_set_state(STATE_LISTENING);
    telemetry_test_set_raw_adc(1000, 2500);

    mock_tick = 1000;
    fsm_app_update();

    mock_tick = 4000;
    fsm_app_update();
    TEST_ASSERT_EQUAL(STATE_LISTENING, fsm_test_get_state());

    telemetry_test_set_raw_adc(2000, 2000);
    mock_tick = 4500;
    fsm_app_update();

    telemetry_test_set_raw_adc(1000, 2500);
    mock_tick = 5000;
    fsm_app_update();

    mock_tick = 7000;
    fsm_app_update();
    TEST_ASSERT_EQUAL(STATE_LISTENING, fsm_test_get_state());
}

void test_fsm_warning_to_waiting_on_long_press(void)
{
    fsm_test_set_state(STATE_WARNING);
    mock_pin13_val = 1;
    mock_tick = 1000;
    fsm_app_update();

    mock_pin13_val = 0;
    mock_tick = 2000;
    fsm_app_update();

    mock_tick = 4100;
    fsm_app_update();
    TEST_ASSERT_EQUAL(STATE_WARNING, fsm_test_get_state());

    mock_pin13_val = 1;
    mock_tick = 4200;
    fsm_app_update();
    TEST_ASSERT_EQUAL(STATE_WAITING, fsm_test_get_state());
}

void test_fsm_warning_to_fatal_on_10s_timeout(void)
{
    fsm_test_set_state(STATE_WARNING);
    mock_pin13_val = 1;
    mock_tick = 1000;
    fsm_app_update();

    mock_tick = 11050;
    fsm_app_update();
    TEST_ASSERT_EQUAL(STATE_FATAL, fsm_test_get_state());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_fsm_initial_state_is_post);
    RUN_TEST(test_fsm_post_settling_delay_below_100ms);
    RUN_TEST(test_fsm_post_success_transitions_to_waiting);
    RUN_TEST(test_fsm_post_pot_out_of_bounds_transitions_to_fatal);
    RUN_TEST(test_fsm_post_ntc_out_of_bounds_transitions_to_fatal);
    RUN_TEST(test_fsm_waiting_to_listening_on_button_press);
    RUN_TEST(test_fsm_waiting_to_listening_on_cli_run);
    RUN_TEST(test_fsm_listening_to_paused_on_button_press);
    RUN_TEST(test_fsm_paused_to_listening_on_cli_run);
    RUN_TEST(test_fsm_listening_to_warning_after_5s_continuous_diff);
    RUN_TEST(test_fsm_listening_diff_drop_resets_warning_timer);
    RUN_TEST(test_fsm_warning_to_waiting_on_long_press);
    RUN_TEST(test_fsm_warning_to_fatal_on_10s_timeout);

    return UNITY_END();
}