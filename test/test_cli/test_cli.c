#include <unity.h>
#include "recruiting/cli/cli.h"
#include "recruiting/telemetry/telemetry.h"

void setUp(void)
{
    cli_test_reset();
}

void tearDown(void)
{
}

void test_cli_command_raw(void)
{
    telemetry_set_mode(AVG);
    cli_test_inject_string("raw\r");

    TEST_ASSERT_EQUAL(RAW, cli_test_get_last_mode());
    TEST_ASSERT_FALSE(cli_input_run());
}

void test_cli_command_moving_average(void)
{
    cli_test_inject_string("moving average\n");
    TEST_ASSERT_EQUAL(AVG, cli_test_get_last_mode());
    TEST_ASSERT_FALSE(cli_input_run());
}

void test_cli_command_random_noise(void)
{
    TEST_ASSERT_EQUAL_INT(0, cli_test_get_noise_toggle_count());
    cli_test_inject_string("random noise\r");
    TEST_ASSERT_EQUAL_INT(1, cli_test_get_noise_toggle_count());
    cli_test_inject_string("random noise\n");
    TEST_ASSERT_EQUAL_INT(2, cli_test_get_noise_toggle_count());
}

void test_cli_command_run_latching(void)
{
    TEST_ASSERT_FALSE(cli_input_run());
    cli_test_inject_string("run\r");
    TEST_ASSERT_TRUE(cli_input_run());
    TEST_ASSERT_FALSE(cli_input_run());
}

void test_cli_unknown_command(void)
{
    cli_test_inject_string("calibrate\r");
    TEST_ASSERT_FALSE(cli_input_run());
    TEST_ASSERT_EQUAL_INT(0, cli_test_get_noise_toggle_count());
}

void test_cli_empty_newline_does_nothing(void)
{
    cli_test_inject_string("\r\n");
    TEST_ASSERT_FALSE(cli_input_run());
    TEST_ASSERT_EQUAL_INT(0, cli_test_get_noise_toggle_count());
}

void test_cli_buffer_overflow_protection(void)
{
    for (int i = 0; i < 40; i++)
    {
        cli_test_inject_char('x');
    }
    cli_test_inject_char('\r');
    TEST_ASSERT_FALSE(cli_input_run());
    cli_test_inject_string("run\n");
    TEST_ASSERT_TRUE(cli_input_run());
}

void test_cli_print_menu_safe_execution(void)
{
    cli_print_menu();
    TEST_ASSERT_TRUE(true);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_cli_command_raw);
    RUN_TEST(test_cli_command_moving_average);
    RUN_TEST(test_cli_command_random_noise);
    RUN_TEST(test_cli_command_run_latching);
    RUN_TEST(test_cli_unknown_command);
    RUN_TEST(test_cli_empty_newline_does_nothing);
    RUN_TEST(test_cli_buffer_overflow_protection);
    RUN_TEST(test_cli_print_menu_safe_execution);

    return UNITY_END();
}