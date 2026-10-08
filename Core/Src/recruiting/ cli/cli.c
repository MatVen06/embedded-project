#include "recruiting/cli/cli.h"
#include "recruiting/telemetry/telemetry.h"
#include "usart.h"

static uint8_t cli_byte;
static volatile bool cli_byte_received = false;
static char cli_buffer[CLI_BUFFER_SIZE];
static uint8_t cli_buffer_index = 0;
static bool run_command_received = false;

void cli_init(void)
{
    HAL_UART_Receive_IT(&huart2, &cli_byte, 1);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        cli_byte_received=true;
        HAL_UART_Receive_IT(&huart2, &cli_byte, 1);
    }
}

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
                    telemetry_set_mode(RAW);
                }
                else if(strcmp(cli_buffer, "moving average") == 0)
                {
                    telemetry_set_mode(AVG);
                }
                else if(strcmp(cli_buffer, "random noise") == 0)
                {
                    telemetry_noise_toggle();
                }
                else if(strcmp(cli_buffer, "run")==0)
                {
                    run_command_received = true;
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
        "  moving average -> Stream 100-sample filtered average\r\n"
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