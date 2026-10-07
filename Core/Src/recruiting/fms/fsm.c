#include "recruiting/fsm/fsm.h"
#include "recruiting/telemetry/telemetry.h"
#include "main.h"

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


static struct Transition post_transitions[] = 
{
    { .to = STATE_WAITING, .function = NULL },
    { .to = STATE_FATAL, .function = NULL },
};

static struct Transition waiting_transitions[] = 
{
    { .to = STATE_LISTENING, .function = NULL },
    { .to = STATE_FATAL, .function = NULL },
};
static struct Transition listening_transitions[] = 
{
    { .to = STATE_WARNING, .function = NULL },
    { .to = STATE_FATAL, .function = NULL },
    { .to = STATE_PAUSED, .function = NULL },
};

static struct Transition warning_transitions[] = 
{
    { .to = STATE_WAITING, .function = NULL },
    { .to = STATE_FATAL, .function = NULL },
};

static struct Transition paused_transitions[] = 
{
    { .to = STATE_LISTENING, .function = NULL },
    { .to = STATE_FATAL, .function = NULL },
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
        .transitions = NULL,
        .num_transitions = 0,
    },
};

void fsm_app_init(void)
{
    fsm_ctx.time_over_threshold = 0;
    fsm_ctx.uart_last_tx_time = 0;
    fsm_ctx.warning_press_time = 0;
    fsm_ctx.warning_start_time = 0;
    fsm_ctx.error_description = NULL;

    fsm_api_init(&fsm_ctx.handler, states, STATE_COUNT, STATE_POST);
}

void fsm_app_update(void)
{
    fsm_api_run_state(&fsm_ctx.handler, &fsm_ctx);
}


static void routine_post(void* data)
{

    struct FSMcontext *ctx = (struct FSMcontext *)data;

    if(HAL_GetTick() <100)
    {
        return;
    }
    uint16_t pot_raw = telemetry_get_raw_pot();
    uint16_t ntc_raw = telemetry_get_raw_ntc();
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
    
}
