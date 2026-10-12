#ifndef BUTTON_H
#define BUTTON_H



#include<stdbool.h>
#include<stdint.h>

    /*!
     * \brief scans for button presses
     */
    void button_state_update(void);

    /*!
     * \brief detects whether the button was pressed or not
     */
    bool button_was_pressed(void);

#ifdef UNIT_TEST
    #ifndef GPIO_PIN_13
    #define GPIO_PIN_13 ((uint16_t)0x2000U)
    #endif
    /*!
    * \brief Simulates the hardware EXTI falling-edge interrupt
     */
    void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_PIN);

    /*!
     * \brief Resets internal button debounce timer and state flags before each unit test
     */
    void button_test_reset(void);
#endif

#endif