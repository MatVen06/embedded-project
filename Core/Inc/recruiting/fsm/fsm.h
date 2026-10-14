#ifndef FSM_H
#define FSM_H

#include<stdint.h>
#include<stdbool.h>
#include<stddef.h>
#include "eagletrt-fsm-api.h"

/*!
 * \brief Enumeration of all discrete states in the application finite state machine.
 */
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

/*!
 * \brief Master application context structure passed to all FSM routine callbacks[cite: 15].
 */
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

/*!
 * \brief Initializes the FSM context variables, submodules, and registers the state machine engine[cite: 15, 16].
 */
void fsm_app_init(void);
/*!
 * \brief Executes one step of the current state routine and evaluates pending transitions[cite: 15, 16].
 */
void fsm_app_update(void);
/*!
 * \brief Dynamically configures the differential threshold required to trigger the Warning state[cite: 14].
 * \param warning_threshold Difference in raw ADC counts (clamped between 50 and 4095)[cite: 14].
 */
void fsm_set_warning_threshold(uint16_t warning_threshold);
/*!
 * \brief Retrieves the active differential threshold for the Warning state[cite: 14].
 * \return Current threshold value in raw ADC counts[cite: 14].
 */
uint16_t fsm_get_warning_threshold(void);

#ifdef UNIT_TEST
    /*!
     * \brief Returns the current active state ID during native unit testing[cite: 16].
     * \return Active fsm_state_id enum value[cite: 16].
     */
    enum fsm_state_id fsm_test_get_state(void);
    /*!
     * \brief Manually forces the FSM into a specific state for test staging[cite: 16].
     * \param state Target state to stage into the context[cite: 16].
     */
    void fsm_test_set_state(enum fsm_state_id state);
    /*!
     * \brief Returns a pointer to the static application context for test assertions[cite: 16].
     * \return Pointer to the active struct FSMcontext instance[cite: 16].
     */
    struct FSMcontext* fsm_test_get_context(void);
    /*!
     * \brief Resets the FSM context and restores default state machine configuration before a test[cite: 16].
     */
    void fsm_test_reset(void);
#endif

#endif