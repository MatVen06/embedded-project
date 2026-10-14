#include "recruiting/fsm/fsm.h"
#include "recruiting/telemetry/telemetry.h"
#include "recruiting/button/button.h"
#include "recruiting/cli/cli.h"
#include <string.h>
#include <stdio.h>

#ifndef UNIT_TEST
#include "tim.h"
#include "main.h"
#include "usart.h"
#else
#ifndef GPIO_PIN_5
#define GPIO_PIN_5 ((uint16_t)0x0020U)
#endif
#ifndef GPIO_PIN_13
#define GPIO_PIN_13 ((uint16_t)0x2000U)
#endif
#ifndef GPIO_PIN_SET
#define GPIO_PIN_SET 1
#endif
#ifndef GPIO_PIN_RESET
#define GPIO_PIN_RESET 0
#endif
#ifndef HAL_MAX_DELAY
#define HAL_MAX_DELAY 0xFFFFFFFFU
#endif
#ifndef TIM_CHANNEL_1
#define TIM_CHANNEL_1 0x00000000U
#endif

#define GPIOA ((void*)1)
#define GPIOC ((void*)2)

#define HAL_TIM_PWM_Start(...) ((void)0)
#define __HAL_TIM_MOE_ENABLE(...) ((void)0)
#define HAL_TIM_Base_Start_IT(...) ((void)0)
#define HAL_TIM_Base_Stop_IT(...) ((void)0)
#define __HAL_TIM_SET_AUTORELOAD(...) ((void)0)
#define __HAL_TIM_SET_COUNTER(...) ((void)0)
#define __HAL_TIM_SET_COMPARE(...) ((void)0)
#define HAL_GPIO_WritePin(...) ((void)0)
#define HAL_UART_Transmit(...) ((void)0)

uint32_t HAL_GetTick(void);

__attribute__((weak)) int HAL_GPIO_ReadPin(void *port, uint16_t pin)
{
    (void)port;
    (void)pin;
    return 1;
}

__attribute__((weak)) void NVIC_SystemReset(void) {}
#endif

#define POST_NTC_RAW_MIN 100
#define POST_NTC_RAW_MAX 4000

#define POST_POT_RAW_MIN 0
#define POST_POT_RAW_MAX 4085

static uint16_t fsm_warning_threshold = 1000;

static uint32_t fsm_led_on_time = 75;
static uint32_t fsm_led_off_time = 425;
static bool long_btn_press = false;

static struct FSMcontext fsm_ctx;

static void routine_post(void* data);

static void routine_waiting(void* data);
static void routine_listening(void* data);
static void routine_paused(void* data);
static void routine_warning(void* data);
static void routine_fatal(void* data);

static void dummy_transition(void* data)
{ 
    (void)data;
}


static struct Transition post_transitions[] = 
{
    { .to = STATE_WAITING, .function = dummy_transition},
    { .to = STATE_FATAL, .function =  dummy_transition},
};

static struct Transition waiting_transitions[] = 
{
    { .to = STATE_LISTENING, .function = dummy_transition},
    { .to = STATE_FATAL, .function = dummy_transition },
};
static struct Transition listening_transitions[] = 
{
    { .to = STATE_WARNING, .function = dummy_transition},
    { .to = STATE_FATAL, .function = dummy_transition},
    { .to = STATE_PAUSED, .function = dummy_transition},
};

static struct Transition warning_transitions[] = 
{
    { .to = STATE_WAITING, .function = dummy_transition },
    { .to = STATE_FATAL, .function = dummy_transition },
};

static struct Transition paused_transitions[] = 
{
    { .to = STATE_LISTENING, .function = dummy_transition },
    { .to = STATE_FATAL, .function = dummy_transition},
};
static struct Transition fatal_transitions[] = 
{
    { .to = STATE_FATAL, .function = dummy_transition }
};


static struct State states[] = {
    {
        .id = STATE_POST,
        .function = routine_post,
        .next_default = STATE_POST, // This is a repeat state
        .transitions = post_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(post_transitions),
    },
    {
        .id = STATE_WAITING,
        .function = routine_waiting,
        .next_default = STATE_WAITING, // This is a repeat state
        .transitions = waiting_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(waiting_transitions),
    },
    {
        .id = STATE_LISTENING,
        .function = routine_listening,
        .next_default = STATE_LISTENING, // This is a repeat state
        .transitions = listening_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(listening_transitions),
    },
    {
        .id = STATE_WARNING,
        .function = routine_warning,
        .next_default = STATE_WARNING, // This is a repeat state
        .transitions = warning_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(warning_transitions),
    },
    {
        .id = STATE_PAUSED,
        .function = routine_paused,
        .next_default = STATE_PAUSED, // This is a repeat state
        .transitions = paused_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(paused_transitions),
    },
    {
        .id = STATE_FATAL,
        .function = routine_fatal,
        .next_default = STATE_FATAL, // This is a repeat state
        .transitions = fatal_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(fatal_transitions),
    },
};

#ifdef UNIT_TEST

    enum fsm_state_id fsm_test_get_state(void)
    {
        return (enum fsm_state_id)fsm_ctx.handler.current_state;
    }
    void fsm_test_set_state(enum fsm_state_id state)
    {
        fsm_ctx.handler.current_state = (uint8_t)state;
        fsm_ctx.handler.requested_state = (uint8_t)state;
        fsm_ctx.previous_state = STATE_COUNT;
        fsm_ctx.time_over_threshold = 0;
        fsm_ctx.warning_start_time = 0;
        fsm_ctx.warning_press_time = 0;
        long_btn_press = false;

    }
    struct FSMcontext* fsm_test_get_context(void)
    {
        return &fsm_ctx;
    }
    void fsm_test_reset(void)
    {
        fsm_app_init();
    }

#endif




void fsm_app_init(void)
{
    fsm_warning_threshold = 1000;
    fsm_ctx.time_over_threshold = 0;
    fsm_ctx.uart_last_tx_time = 0;
    fsm_ctx.warning_press_time = 0;
    fsm_ctx.warning_start_time = 0;
    fsm_ctx.error_description = NULL;
    fsm_ctx.previous_state = STATE_COUNT;
    cli_init();
    telemetry_init();
    fsm_api_init(&fsm_ctx.handler, states, STATE_COUNT, STATE_POST);
}

void fsm_app_update(void)
{    
    fsm_api_run_state(&fsm_ctx.handler, &fsm_ctx);
}


static void routine_post(void* data)
{
   // HAL_UART_Transmit(&huart2, (uint8_t*)"[POST Running]\r\n", 16, 100);
    struct FSMcontext *ctx = (struct FSMcontext *)data;

    if(HAL_GetTick() <100)
    {
        return;
    }
    uint16_t pot_raw = telemetry_get_raw_pot();
    uint16_t ntc_raw = telemetry_get_raw_ntc();
  //  char tx_buffer[64];
  //  int len = snprintf(tx_buffer, sizeof(tx_buffer), "POT: %u, NTC: %u\r\n", pot_raw, ntc_raw);
   // HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len, 100);


    if(pot_raw < POST_POT_RAW_MIN || pot_raw >= POST_POT_RAW_MAX)
    {
        ctx->error_description = "POST Error: POT out of range";
        fsm_api_transition_to(&ctx->handler, STATE_FATAL);
    }
    else if(ntc_raw <= POST_NTC_RAW_MIN || ntc_raw >= POST_NTC_RAW_MAX)
    {
        ctx->error_description = "POST Error: NTC out of range";
        fsm_api_transition_to(&ctx->handler, STATE_FATAL);
    }
    else
    {
        fsm_api_transition_to(&ctx->handler, STATE_WAITING);
    }
}

#ifndef UNIT_TEST
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14)
    {
        if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET)
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
            __HAL_TIM_SET_AUTORELOAD(htim, fsm_led_off_time - 1);
        }
        else
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
            __HAL_TIM_SET_AUTORELOAD(htim, fsm_led_on_time - 1);      
        }
    }

}
#endif

void fsm_set_warning_threshold(uint16_t warning_threshold)
{
    if(warning_threshold < 50)
    {
        fsm_warning_threshold = 50;
        return;
    }
    else if(warning_threshold > 4095)
    {
        fsm_warning_threshold = 4095;
        return;
    }
    fsm_warning_threshold = warning_threshold;
}

uint16_t fsm_get_warning_treshold()
{
    return fsm_warning_threshold;
}




static void routine_waiting(void* data)
{
    struct FSMcontext *ctx = (struct FSMcontext *)data;
    if(ctx->previous_state != STATE_WAITING)
    {
        ctx->previous_state = STATE_WAITING;
        fsm_led_off_time = 425;
        fsm_led_on_time = 75;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_TIM_Base_Start_IT(&htim14);
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
        __HAL_TIM_MOE_ENABLE(&htim1);
        cli_print_menu();
  
    

    }
    cli_update();
    button_state_update();
    uint32_t t = HAL_GetTick() % 2000;
    uint32_t compare_val = 0;

    if(t<=1000)
    {
        compare_val = (t*999)/1000; 
    }
    else
    {
        compare_val = ((2000-t)*999)/1000;
    }
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, compare_val);

    if(button_was_pressed() || cli_input_run())
    {
        fsm_api_transition_to(&ctx->handler, STATE_LISTENING);
    }
    
}
static void routine_listening(void* data)
{
    struct FSMcontext *ctx = (struct FSMcontext *)data;
    if (ctx->previous_state != STATE_LISTENING)
    {
        fsm_led_off_time = 40;
        fsm_led_on_time = 160;
        ctx->time_over_threshold = 0;
        ctx->previous_state = STATE_LISTENING;
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
        __HAL_TIM_MOE_ENABLE(&htim1);
        HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n--- Entered LISTENING State ---\r\n", 35, HAL_MAX_DELAY);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        __HAL_TIM_SET_AUTORELOAD(&htim14, fsm_led_on_time - 1);
        __HAL_TIM_SET_COUNTER(&htim14, 0);
    }
    button_state_update();
    uint16_t pot_raw = telemetry_get_raw_pot();
    uint16_t ntc_raw = telemetry_get_raw_ntc();
    uint16_t diff = 0;
    uint16_t compare_val = 0;
    if(ntc_raw <= pot_raw)
    {
        compare_val = 0;
        ctx->time_over_threshold = 0;
    }
    else if(ntc_raw > pot_raw)
    {
        diff = ntc_raw - pot_raw;
        if(diff >= fsm_warning_threshold)
        {
            compare_val = 999;
            if(ctx->time_over_threshold == 0)
            {
                ctx->time_over_threshold = HAL_GetTick();
            }
            else if(HAL_GetTick() - ctx->time_over_threshold >= 5000)
            {
                ctx->time_over_threshold = 0;
                fsm_api_transition_to(&ctx->handler, STATE_WARNING);
            }
        }
        else 
        {
            ctx->time_over_threshold = 0;
            compare_val = (diff*999)/fsm_warning_threshold;
        }
    }
    if(HAL_GetTick() - ctx->uart_last_tx_time >= 100)
    {
        ctx->uart_last_tx_time = HAL_GetTick();
        telemetry_update();
    }

    if(button_was_pressed())
    {
        ctx->time_over_threshold = 0;
        fsm_api_transition_to(&ctx->handler, STATE_PAUSED);
    }

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, compare_val);
}
static void routine_warning(void* data)
{
    struct FSMcontext (*ctx) = (struct FSMcontext*)data;
    if(ctx->previous_state != STATE_WARNING)
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n--- Entered WARNING State ---\r\n", 35, HAL_MAX_DELAY);
        ctx->previous_state = STATE_WARNING;
        long_btn_press = false;
        button_was_pressed();
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        HAL_TIM_Base_Stop_IT(&htim14);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        ctx->warning_start_time = HAL_GetTick();
        ctx->warning_press_time = 0;
    }  

    if(HAL_GetTick() - ctx->uart_last_tx_time >= 200)
    {
        ctx->uart_last_tx_time = HAL_GetTick();
        HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nWARNING\r\n", 11, HAL_MAX_DELAY);
    }

    if(HAL_GetTick() - ctx->warning_start_time >= 10000)
    {
        ctx->error_description = "Warning state for 10+ seconds -> transitioning to fatal state";
        fsm_api_transition_to(&ctx->handler, STATE_FATAL);
        return;
    }

    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
    {
        if(ctx->warning_press_time == 0)
        {
            ctx->warning_press_time = HAL_GetTick();
        }
        else if(HAL_GetTick() - ctx->warning_press_time >= 2000)
        {
            long_btn_press = true;
        }
    }
    else
    {
        if(long_btn_press)
        {
            long_btn_press = false;
            ctx->warning_press_time = 0;
            button_was_pressed();
            fsm_api_transition_to(&ctx->handler, STATE_WAITING);
            return;
        }
        button_was_pressed();
        ctx->warning_press_time = 0;
    }   
}
static void routine_paused(void* data)
{
    struct FSMcontext (*ctx) = (struct FSMcontext*)data;
    if(ctx->previous_state != STATE_PAUSED)
    {
        ctx->previous_state = STATE_PAUSED;
        fsm_led_off_time = 1000;
        fsm_led_on_time = 1000;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        __HAL_TIM_SET_AUTORELOAD(&htim14, fsm_led_on_time - 1);
        __HAL_TIM_SET_COUNTER(&htim14, 0);
        HAL_TIM_Base_Start_IT(&htim14);
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
        __HAL_TIM_MOE_ENABLE(&htim1);
        HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n--- Entered PAUSED State ---\r\n", strlen("\r\n--- Entered PAUSED State ---\r\n"), HAL_MAX_DELAY);
        cli_print_menu();
    }
    cli_update();
    button_state_update();
    uint32_t t = HAL_GetTick() % 2000;
    uint32_t compare_val = 0;

    if(t<=1000)
    {
        compare_val = (t*999)/1000; 
    }
    else
    {
        compare_val = ((2000-t)*999)/1000;
    }
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, compare_val);

    if(button_was_pressed() || cli_input_run())
    {
        fsm_api_transition_to(&ctx->handler, STATE_LISTENING);
    }
}




static void routine_fatal(void* data)
{
    struct FSMcontext *ctx = (struct FSMcontext *)data;
    if(ctx->previous_state != STATE_FATAL)
    {
        ctx->previous_state = STATE_FATAL;
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        fsm_led_off_time = 20;
        fsm_led_on_time = 20;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); 
        __HAL_TIM_SET_AUTORELOAD(&htim14, fsm_led_on_time - 1);
        __HAL_TIM_SET_COUNTER(&htim14, 0);
        HAL_TIM_Base_Start_IT(&htim14);
    }

    if(HAL_GetTick() - ctx->uart_last_tx_time >= 100)
    {
        ctx->uart_last_tx_time = HAL_GetTick();
        char error_msg[128];
        int len = snprintf(error_msg, sizeof(error_msg), "ERROR: %s\r\n", 
                   ctx->error_description ? ctx->error_description : "Unknown error");
        HAL_UART_Transmit(&huart2, (uint8_t*)error_msg, len, 100);
    }

    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
    {
        NVIC_SystemReset();
    }

}