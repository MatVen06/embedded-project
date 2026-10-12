#include<unity.h>
#include"recruiting/telemetry/telemetry.h"

void setUp(void)
{
    telemetry_filter_reset();
}

void tearDown(void)
{

}

void test_telemetry_pot_to_celsius_boundaries(void)
{
    float temp_min = telemetry_pot_to_celsius(0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, temp_min);

    float temp_max = telemetry_pot_to_celsius(4095);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, temp_max);

    float temp_mid = telemetry_pot_to_celsius(2048);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, temp_mid);
}
void test_telemetry_ntc_to_celsius_boundaries(void)
{
    float ntc_min = telemetry_ntc_to_celsius(0);
    TEST_ASSERT_FALSE(isnan(ntc_min));
    TEST_ASSERT_FALSE(isinf(ntc_min));
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 527.8f, ntc_min);

    float ntc_max = telemetry_ntc_to_celsius(4095);
    TEST_ASSERT_FALSE(isnan(ntc_max));
    TEST_ASSERT_FALSE(isinf(ntc_max));
    TEST_ASSERT_FLOAT_WITHIN(1.0f, -90.0f, ntc_max);

    float ntc_mid = telemetry_ntc_to_celsius(2048);
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 25.0f, ntc_mid);
}
void test_telemetry_filter_baseline_accumulation(void)
{
    telemetry_filter_add_sample(100, 500);
    telemetry_filter_add_sample(200, 600);
    telemetry_filter_add_sample(300, 700);

    TEST_ASSERT_EQUAL_UINT16(200, telemetry_filter_get_pot_avg());
    TEST_ASSERT_EQUAL_UINT16(600, telemetry_filter_get_ntc_avg());
}
void test_telemetry_filter_circular_rollover(void)
{
    for(int i = 0; i<150; i++)
    {
        telemetry_filter_add_sample(100, 100);
    }
    TEST_ASSERT_EQUAL_UINT16(100, telemetry_filter_get_pot_avg());
    
    telemetry_filter_add_sample(250, 250);
    TEST_ASSERT_EQUAL_UINT16(101, telemetry_filter_get_pot_avg());
    TEST_ASSERT_EQUAL_UINT16(101, telemetry_filter_get_ntc_avg());
}

void test_telemetry_filter_reset_and_empty_buffer(void)
{
    telemetry_filter_add_sample(1500, 2500);
    telemetry_filter_add_sample(1600, 2600);
    telemetry_filter_reset();
    TEST_ASSERT_EQUAL_UINT16(0, telemetry_filter_get_pot_avg());
    TEST_ASSERT_EQUAL_UINT16(0, telemetry_filter_get_ntc_avg());
}

void test_telemetry_filter_exact_150_sample_window(void)
{
    for (int i = 0; i < 75; i++)
    {
        telemetry_filter_add_sample(200, 1000);
    }
    for (int i = 0; i < 75; i++)
    {
        telemetry_filter_add_sample(400, 3000);
    }
    TEST_ASSERT_EQUAL_UINT16(300, telemetry_filter_get_pot_avg());
    TEST_ASSERT_EQUAL_UINT16(2000, telemetry_filter_get_ntc_avg());
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_telemetry_pot_to_celsius_boundaries);
    RUN_TEST(test_telemetry_ntc_to_celsius_boundaries);
    RUN_TEST(test_telemetry_filter_baseline_accumulation);
    RUN_TEST(test_telemetry_filter_circular_rollover);
    RUN_TEST(test_telemetry_filter_reset_and_empty_buffer);
    RUN_TEST(test_telemetry_filter_exact_150_sample_window);

    return UNITY_END();
}