#include "recruiting/cli/cli.h"
#include "recruiting/telemetry/telemetry.h"
#ifndef UNIT_TEST
#include "usart.h"
#else
/* Stub out HAL functions during native unit tests */
#define HAL_UART_Transmit(huart, pData, Size, Timeout) ((void)0)
#define HAL_UART_Receive_IT(huart, pData, Size)        ((void)0)
#endif


static uint8_t cli_byte;
static volatile bool cli_byte_received = false;
static char cli_buffer[CLI_BUFFER_SIZE];
static uint8_t cli_buffer_index = 0;
static bool run_command_received = false;

#ifdef UNIT_TEST
static enum telemetry_mode cli_test_last_mode = RAW;
static int cli_test_noise_toggle_count = 0;


enum telemetry_mode cli_test_get_last_mode(void)
{
    return cli_test_last_mode;
}

int cli_test_get_noise_toggle_count(void)
{
    return cli_test_noise_toggle_count;
}

void cli_test_inject_char(char c)
{
    cli_byte = (uint8_t)c;
    cli_byte_received = true;
    cli_update();
}

void cli_test_inject_string(const char *str)
{
    while(*str)
    {
        cli_test_inject_char(*str++);
    }
}

void cli_test_reset(void)
{
    cli_buffer_index = 0;
    cli_byte_received = false;
    run_command_received = false;
    cli_test_last_mode = RAW;
    cli_test_noise_toggle_count = 0;
}
#endif


void cli_init(void)
{
    #ifndef UNIT_TEST
        HAL_UART_Receive_IT(&huart2, &cli_byte, 1);
    #else
        cli_test_reset();
    #endif
}

#ifndef UNIT_TEST
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        cli_byte_received=true;
        HAL_UART_Receive_IT(&huart2, &cli_byte, 1);
    }
}
#endif
void cli_update(void)
{
    if(cli_byte_received == true)
    {
        cli_byte_received = false;
        if(cli_byte == '\r' || cli_byte == '\n')
        {

            if(cli_buffer_index > 0)
            {

                cli_buffer[cli_buffer_index] = '\0';

                if(strcmp(cli_buffer, "raw") == 0)
                {
                    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nMode set to RAW\r\n", 19, HAL_MAX_DELAY);
                    telemetry_set_mode(RAW);
#ifdef UNIT_TEST
                    cli_test_last_mode = RAW;
#endif
                }
                else if(strcmp(cli_buffer, "moving average") == 0)
                {
                    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nToggled moving average\r\n", 26, HAL_MAX_DELAY);
                    telemetry_set_mode(AVG);
#ifdef UNIT_TEST
                    cli_test_last_mode = AVG;
#endif
                }
                else if(strcmp(cli_buffer, "random noise") == 0)
                {
                    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nToggled random noise\r\n", 24, HAL_MAX_DELAY);
                    telemetry_noise_toggle();

#ifdef UNIT_TEST
                    cli_test_noise_toggle_count++;
#endif
                }
                else if(strcmp(cli_buffer, "run")==0)
                {
                    run_command_received = true;
                }
                else
                {
                    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nUnknown command\r\n", 19, HAL_MAX_DELAY);
                }
                cli_buffer_index=0;
            }
        }
        else 
        {
            if(cli_buffer_index < (CLI_BUFFER_SIZE - 1))
            {
                cli_buffer[cli_buffer_index] = (char)cli_byte;
                cli_buffer_index++;

                // Echo the typed character back to the terminal
                HAL_UART_Transmit(&huart2, &cli_byte, 1, HAL_MAX_DELAY);
            }
        }
    }
}


void cli_print_menu(void)
{
    const char menu_text[] = 
        "\r\n--- CLI MODE ACTIVE ---\r\n"
        "Available commands:\r\n"
        "  raw            -> Stream raw data (disables noise)\r\n"
        "  moving average -> Stream 150-sample filtered average\r\n"
        "  random noise   -> Toggle artificial noise\r\n"
        "  run            -> Start streaming (enter Listening state)\r\n"
        "Press button to resume streaming.\r\n"
        "> ";

    HAL_UART_Transmit(&huart2, (uint8_t*)menu_text, sizeof(menu_text) - 1, HAL_MAX_DELAY);
}

bool cli_input_run(void)
{
    if(run_command_received)
    {
        run_command_received = false;
        return true;
    }
    return false;
}