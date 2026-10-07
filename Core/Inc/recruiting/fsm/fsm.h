#ifndef FSM_H
#define FSM_H

#include<stdint.h>
#include<stdbool.h>
#include<stddef.h>
#include "eagletrt-fsm-api.h"

enum fsm_state_id
{
    STATE_POST,
    STATE_WAITING,
    STATE_LISTENING,
    STATE_PAUSED,
    STATE_WARNING,
    STATE_FATAL,
    STATE_COUNT
};

struct FSMcontext
{
    struct FSMHandler handler;
    uint32_t time_over_threshold;
    uint32_t warning_start_time;
    uint32_t warning_press_time;
    uint32_t uart_last_tx_time;
    const char* error_description;
};


void fsm_app_init(void);

void fsm_app_update(void);

#endif