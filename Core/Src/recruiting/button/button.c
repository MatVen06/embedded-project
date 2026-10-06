#include "recruiting/button/button.h"
#include "main.h"

static volatile bool button_EXTI1_triggered = false;
static bool button_state = false;
static uint32_t button_history;

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
            button_state=!button_state;
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); 
 

        }
    }
}

bool button_cli_mode(void) 
{
    return button_state;
}