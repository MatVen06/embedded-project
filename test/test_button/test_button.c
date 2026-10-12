#include <unity.h>
#include "fff.h"
#include "recruiting/button/button.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC0(uint32_t, HAL_GetTick);

void setUp(void)
{
    RESET_FAKE(HAL_GetTick);
    FFF_RESET_HISTORY();
    button_test_reset();
    HAL_GetTick_fake.return_val = 1000;
}

void tearDown(void)
{
}

void test_button_init_state_not_pressed(void)
{
    TEST_ASSERT_FALSE(button_was_pressed());
}

void test_button_single_valid_press(void)
{
    HAL_GetTick_fake.return_val = 1000;
    HAL_GPIO_EXTI_Falling_Callback(GPIO_PIN_13);
    button_state_update();
    TEST_ASSERT_TRUE(button_was_pressed());
}

void test_button_read_and_clear_latch(void)
{
    HAL_GetTick_fake.return_val = 1000;
    HAL_GPIO_EXTI_Falling_Callback(GPIO_PIN_13);
    button_state_update();
    TEST_ASSERT_TRUE(button_was_pressed());
    TEST_ASSERT_FALSE(button_was_pressed());
}

void test_button_debounce_filters_rapid_glitches(void)
{
    /* Initial valid press at t = 1000 */
    HAL_GetTick_fake.return_val = 1000;
    HAL_GPIO_EXTI_Falling_Callback(GPIO_PIN_13);
    button_state_update();
    TEST_ASSERT_TRUE(button_was_pressed());

    /* Glitch bounce at t = 1050 (50 ms < 200 ms) */
    HAL_GetTick_fake.return_val = 1050;
    HAL_GPIO_EXTI_Falling_Callback(GPIO_PIN_13);
    button_state_update();
    TEST_ASSERT_FALSE(button_was_pressed());

    /* Glitch bounce at t = 1199 (199 ms < 200 ms) */
    HAL_GetTick_fake.return_val = 1199;
    HAL_GPIO_EXTI_Falling_Callback(GPIO_PIN_13);
    button_state_update();
    TEST_ASSERT_FALSE(button_was_pressed());
}

void test_button_second_press_after_200ms_accepted(void)
{
    HAL_GetTick_fake.return_val = 1000;
    HAL_GPIO_EXTI_Falling_Callback(GPIO_PIN_13);
    button_state_update();
    TEST_ASSERT_TRUE(button_was_pressed());
    HAL_GetTick_fake.return_val = 1200;
    HAL_GPIO_EXTI_Falling_Callback(GPIO_PIN_13);
    button_state_update();
    TEST_ASSERT_TRUE(button_was_pressed());
}

void test_button_foreign_pin_interrupt_ignored(void)
{
    HAL_GetTick_fake.return_val = 1000;
    HAL_GPIO_EXTI_Falling_Callback(0x0001);
    button_state_update();

    TEST_ASSERT_FALSE(button_was_pressed());
}

void test_button_update_without_interrupt_does_nothing(void)
{
    HAL_GetTick_fake.return_val = 2000;
    button_state_update();

    TEST_ASSERT_FALSE(button_was_pressed());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_button_init_state_not_pressed);
    RUN_TEST(test_button_single_valid_press);
    RUN_TEST(test_button_read_and_clear_latch);
    RUN_TEST(test_button_debounce_filters_rapid_glitches);
    RUN_TEST(test_button_second_press_after_200ms_accepted);
    RUN_TEST(test_button_foreign_pin_interrupt_ignored);
    RUN_TEST(test_button_update_without_interrupt_does_nothing);

    return UNITY_END();
}