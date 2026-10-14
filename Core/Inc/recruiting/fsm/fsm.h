#ifndef FSM_H
#define FSM_H

#include<stdint.h>
#include<stdbool.h>
#include<stddef.h>
#include "eagletrt-fsm-api.h"

enum fsm_state_id
{
    STATE_POST = 0,
    STATE_WAITING,
    STATE_LISTENING,
    STATE_WARNING,
    STATE_PAUSED,
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
    enum fsm_state_id previous_state;
};


void fsm_app_init(void);

void fsm_app_update(void);

void fsm_set_warning_threshold(uint16_t warning_threshold);

uint16_t fsm_get_warning_threshold(void);

#ifdef UNIT_TEST
    enum fsm_state_id fsm_test_get_state(void);
    void fsm_test_set_state(enum fsm_state_id state);
    struct FSMcontext* fsm_test_get_context(void);
    void fsm_test_reset(void);
#endif

#endif