#include "recruiting/fsm/fsm.h"
#include "recruiting/telemetry/telemetry.h"
#include "recruiting/button/button.h"
#include "recruiting/cli/cli.h"
#include "tim.h"
#include "main.h"
#include "usart.h"
#include <string.h>
#include<stdio.h>

#define POST_NTC_RAW_MIN 100
#define POST_NTC_RAW_MAX 4000

#define POST_POT_RAW_MIN 0
#define POST_POT_RAW_MAX 4085


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
    { .to = STATE_WAITING, .function = dummy_transition },
    { .to = STATE_FATAL, .function =  dummy_transition},
};

static struct Transition waiting_transitions[] = 
{
    { .to = STATE_LISTENING, .function = dummy_transition},
    { .to = STATE_FATAL, .function = dummy_transition },
};
static struct Transition listening_transitions[] = 
{
    { .to = STATE_WARNING, .function = dummy_transition },
    { .to = STATE_FATAL, .function = dummy_transition },
    { .to = STATE_PAUSED, .function = dummy_transition },
};

static struct Transition warning_transitions[] = 
{
    { .to = STATE_WAITING, .function = dummy_transition },
    { .to = STATE_FATAL, .function = dummy_transition },
};

static struct Transition paused_transitions[] = 
{
    { .to = STATE_LISTENING, .function = dummy_transition },
    { .to = STATE_FATAL, .function = dummy_transition },
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

void fsm_app_init(void)
{
    fsm_ctx.time_over_threshold = 0;
    fsm_ctx.uart_last_tx_time = 0;
    fsm_ctx.warning_press_time = 0;
    fsm_ctx.warning_start_time = 0;
    fsm_ctx.error_description = NULL;
    fsm_ctx.previous_state = STATE_COUNT;

    fsm_api_init(&fsm_ctx.handler, states, STATE_COUNT, STATE_POST);
}

void fsm_app_update(void)
{    
   // HAL_UART_Transmit(&huart2, (uint8_t*)"[UPDATE]\r\n", 10, 100);
    //routine_post(&fsm_ctx);  Temporary direct bypass test
    HAL_UART_Transmit(&huart2, (uint8_t*)"[RUN]\r\n", 7, 100);
    fsm_api_run_state(&fsm_ctx.handler, &fsm_ctx);
    HAL_UART_Transmit(&huart2, (uint8_t*)"[DONE]\r\n", 8, 100);
}


static void routine_post(void* data)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)"[POST Running]\r\n", 16, 100);
    struct FSMcontext *ctx = (struct FSMcontext *)data;

    if(HAL_GetTick() <100)
    {
        return;
    }
    uint16_t pot_raw = telemetry_get_raw_pot();
    uint16_t ntc_raw = telemetry_get_raw_ntc();
    char tx_buffer[64];
    int len = snprintf(tx_buffer, sizeof(tx_buffer), "POT: %u, NTC: %u\r\n", pot_raw, ntc_raw);
    HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len, 100);


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

static void routine_waiting(void* data)
{
    struct FSMcontext *ctx = (struct FSMcontext *)data;
    if(ctx->previous_state != STATE_WAITING)
    {
        ctx->previous_state = STATE_WAITING;
        
        // Diagnostic serial print
        char msg[] = "--- FSM: Entered WAITING state ---\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg) - 1, HAL_MAX_DELAY);

        // Corrected timer PWM startup
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 500);
        __HAL_TIM_MOE_ENABLE(&htim1);

        cli_print_menu();
    }
    cli_update();
    /*
    if(button_cli_mode())
    {
        fsm_api_transition_to(&ctx->handler, STATE_LISTENING);

    }
    */
}
static void routine_listening(void* data)
{
    
}
static void routine_warning(void* data)
{
    
}
static void routine_paused(void* data)
{
    
}
static void routine_fatal(void* data)
{
    struct FSMcontext *ctx = (struct FSMcontext *)data;
    if (ctx->error_description != NULL)
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)ctx->error_description, strlen(ctx->error_description), HAL_MAX_DELAY);
        HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
        ctx->error_description = NULL; // Print once
    }
}