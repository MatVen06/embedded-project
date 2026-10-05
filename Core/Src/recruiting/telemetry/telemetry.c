#include "recruiting/telemetry/telemetry.h"
#include "adc.h"
#include "usart.h"
#include<stdio.h>

    static uint16_t telemetry_adc_buffer[TELEMETRY_ADC_BUFFER_SIZE];
    static uint32_t telemetry_last_tx_time = 0;  //time tracker
    static char telemetry_tx_buffer[100]; //formatted txt holder
    void telemetry_init(void)
    {
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)telemetry_adc_buffer, TELEMETRY_ADC_BUFFER_SIZE);
    }

    void telemetry_update(void)
    {
        if ((HAL_GetTick() - telemetry_last_tx_time) >= 100) 
        {
            // Update the timer
            telemetry_last_tx_time = HAL_GetTick();

            // Format the raw data into a string
            int len = sprintf(telemetry_tx_buffer, "POT_RAW:%u, NTC_RAW:%u\r\n", telemetry_adc_buffer[0], telemetry_adc_buffer[1]);

            // Send the string over serial via UART
            HAL_UART_Transmit(&huart2, (uint8_t*)telemetry_tx_buffer, len, HAL_MAX_DELAY);
        }
    }