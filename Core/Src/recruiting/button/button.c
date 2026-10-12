#include "recruiting/button/button.h"

#ifndef UNIT_TEST
#include "main.h"
#else
#ifndef GPIO_PIN_13
#define GPIO_PIN_13 ((uint16_t)0x2000U)
#endif
__attribute__((weak)) uint32_t HAL_GetTick(void)
{
    return 0;
}
#endif
static volatile bool button_EXTI1_triggered = false;
static bool button_state = false;
static uint32_t button_history;

#ifdef UNIT_TEST

void button_test_reset(void)
{
    button_EXTI1_triggered = false;
    button_state = false;
    button_history = 0;
}

void button_test_trigger_press(void)
{
    button_state = true;
}
#endif

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) // captures user button presses
{
    if(GPIO_Pin == GPIO_PIN_13)
    {
        button_EXTI1_triggered = true;
    }
}

void button_state_update(void)
{
    if(button_EXTI1_triggered == true)
    {
        button_EXTI1_triggered = false;
        if(HAL_GetTick() - button_history >=200)
        {
            button_history = HAL_GetTick();
            button_state=true;
        }
    }
}

bool button_was_pressed(void) 
{
    if(button_state)
    {
        button_state = false;
        return true;
    }
    return false;

}