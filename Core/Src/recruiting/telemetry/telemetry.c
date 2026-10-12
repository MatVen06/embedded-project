#include "recruiting/telemetry/telemetry.h"
#include <stdio.h>
#include <math.h>

#ifndef UNIT_TEST
#include "adc.h"
#include "usart.h"
#endif

#define POT_TEMP_MIN    0.0f
#define POT_TEMP_MAX    50.0f

#define NTC_R_FIXED     10000.0f
#define NTC_R0          10000.0f
#define NTC_T0_KELVIN   298.15f
#define NTC_BETA        3950.0f



    static uint16_t telemetry_adc_buffer[TELEMETRY_ADC_BUFFER_SIZE];
    static uint32_t telemetry_last_tx_time = 0;  //time tracker
    static char telemetry_tx_buffer[100]; //formatted txt holder
    static enum telemetry_mode current_mode = RAW;
    static uint16_t telemetry_pot_history[TELEMETRY_WINDOW_SIZE];
    static uint16_t telemetry_ntc_history[TELEMETRY_WINDOW_SIZE];
    static uint8_t telemetry_index_history = 0;
    static bool telemetry_buffer_full = false;
    static bool telemetry_noise_enable = false;


    float telemetry_pot_to_celsius(uint16_t raw_adc)
    {
        return POT_TEMP_MIN + ((float)raw_adc / 4095.0f) * (POT_TEMP_MAX - POT_TEMP_MIN);
    }

    float telemetry_ntc_to_celsius(uint16_t raw_adc)
    {
        if (raw_adc == 0) raw_adc = 1;
        if (raw_adc >= 4095) raw_adc = 4094;

        float r_ntc = NTC_R_FIXED * ((float)raw_adc / (4095.0f - (float)raw_adc));
        float t_kelvin = 1.0f / ((1.0f / NTC_T0_KELVIN) + (1.0f / NTC_BETA) * logf(r_ntc / NTC_R0));

        return t_kelvin - 273.15f;
    }

#ifdef UNIT_TEST

    void telemetry_test_set_raw_adc(uint16_t pot, uint16_t ntc)
    {
        telemetry_adc_buffer[0] = pot;
        telemetry_adc_buffer[1] = ntc;
    }

#endif




    void telemetry_filter_reset(void)
    {
        telemetry_index_history = 0;
        telemetry_buffer_full = false;
        for(int i = 0; i< TELEMETRY_WINDOW_SIZE; i++)
        {
            telemetry_pot_history[i] = 0;
            telemetry_ntc_history[i] = 0;
        }
    }

    void telemetry_filter_add_sample(uint16_t pot_sample, uint16_t ntc_sample)
    {
        telemetry_pot_history[telemetry_index_history] = pot_sample;
        telemetry_ntc_history[telemetry_index_history] = ntc_sample;
        telemetry_index_history++;

        if(telemetry_index_history >= TELEMETRY_WINDOW_SIZE)
        {
            telemetry_buffer_full = true;
            telemetry_index_history = 0;
        }

    }

    uint16_t telemetry_filter_get_pot_avg(void)
    {
        uint32_t pot_sum = 0;
        uint8_t divisor = telemetry_buffer_full ? TELEMETRY_WINDOW_SIZE : telemetry_index_history;
        if(divisor == 0)
        {
            divisor = 1;
        }

        for(int i = 0; i<divisor; i++)
        {
            pot_sum+=telemetry_pot_history[i];
        }

        return (uint16_t)(pot_sum / divisor);
    }

    uint16_t telemetry_filter_get_ntc_avg(void)
    {
        uint32_t ntc_sum = 0;
        uint8_t divisor = telemetry_buffer_full ? TELEMETRY_WINDOW_SIZE : telemetry_index_history;
        if(divisor == 0)
        {
            divisor = 1;
        }

        for(int i = 0; i<divisor; i++)
        {
            ntc_sum+=telemetry_ntc_history[i];
        }

        return (uint16_t)(ntc_sum / divisor);
    }
    #ifndef UNIT_TEST
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

                //values for the noise
                uint16_t pot_raw = telemetry_adc_buffer[0];
                uint16_t ntc_raw = telemetry_adc_buffer[1];
                if(telemetry_noise_enable == true)
                {
                    // Generate noise between -NOISE_AMPLITUDE and +NOISE_AMPLITUDE
                    int16_t pot_noise = (rand() % (2 * NOISE_AMPLITUDE + 1)) - NOISE_AMPLITUDE;
                    int16_t ntc_noise = (rand() % (2 * NOISE_AMPLITUDE + 1)) - NOISE_AMPLITUDE;

                    int32_t noisy_pot = (int32_t)pot_raw + pot_noise;
                    int32_t noisy_ntc = (int32_t)ntc_raw + ntc_noise;

                    // Clamp to valid 12-bit ADC range (0 to 4095)
                    if (noisy_pot < 0)    noisy_pot = 0;
                    if (noisy_pot > 4095) noisy_pot = 4095;
                    if (noisy_ntc < 0)    noisy_ntc = 0;
                    if (noisy_ntc > 4095) noisy_ntc = 4095;

                    pot_raw = (uint16_t)noisy_pot;
                    ntc_raw = (uint16_t)noisy_ntc;
                }

                telemetry_filter_add_sample(pot_raw, ntc_raw);
                uint16_t pot_display_val = 0;
                uint16_t ntc_display_val = 0;
                const char *label_prefix = "";
                if(current_mode == RAW)
                {
                    pot_display_val = pot_raw;
                    ntc_display_val = ntc_raw;
                    label_prefix = "RAW";
                }
                else if(current_mode == AVG)
                {
                    pot_display_val = telemetry_filter_get_pot_avg();
                    ntc_display_val = telemetry_filter_get_ntc_avg();
                    label_prefix = "AVG";
                }
                float pot_c = telemetry_pot_to_celsius(pot_display_val);
                float ntc_c = telemetry_ntc_to_celsius(ntc_display_val);

                int pot_whole = (int)pot_c;
                int pot_frac = (int)((pot_c - pot_whole) * 10.0f);
                if(pot_frac < 0)
                {
                    pot_frac = -pot_frac;
                }
                int ntc_whole = (int)ntc_c;
                int ntc_frac = (int)((ntc_c - ntc_whole) * 10.0f);
                if(ntc_frac < 0)
                {
                    ntc_frac = -ntc_frac;
                }
                int len = sprintf(telemetry_tx_buffer, 
                            "POT_%s:%u, POT_C:%d.%d, NTC_%s:%u, NTC_C:%d.%d\r\n", 
                            label_prefix, pot_display_val, pot_whole, pot_frac, 
                            label_prefix, ntc_display_val, ntc_whole, ntc_frac);

                HAL_UART_Transmit(&huart2, (uint8_t*)telemetry_tx_buffer, len, HAL_MAX_DELAY);
            }   
        }
    #else
            void telemetry_init(void)
            {
                telemetry_filter_reset();
            }
            
            void telemetry_update(void)
            {

            }

    #endif



    void telemetry_set_mode(enum telemetry_mode new_mode)
    {
        current_mode = new_mode;

        if(new_mode==RAW)
        {
            telemetry_noise_enable = false;
        }
    }

    void telemetry_noise_toggle(void)
    {
        telemetry_noise_enable = !telemetry_noise_enable;
    }

    uint16_t telemetry_get_raw_pot(void)
    {
        return telemetry_adc_buffer[0];
    }

    uint16_t telemetry_get_raw_ntc(void)
    {
        return telemetry_adc_buffer[1];
    }