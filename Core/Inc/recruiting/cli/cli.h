#ifndef CLI_H
#define CLI_H
#include <stdint.h>
#include<stdbool.h>
#include<stdio.h>
#include<string.h>


    /*!
     * \brief macro for the starting size of the array that reads the characters in input from the user 
     */
    #define CLI_BUFFER_SIZE 32

    /*!
     * \brief primes the first interrupt when the board boots up   
     */
    void cli_init(void);

    /*!
     * \brief processes incoming characters 
     */
    void cli_update(void);

    /*!
     * \brief shows which filters you can add when you press the button
     */
    void cli_print_menu(void);

    /*!
     * \brief returns true if the word "run" was given as input from the user
     */ 

    bool cli_input_run(void);

    #ifdef UNIT_TEST
    #include "recruiting/telemetry/telemetry.h"
    /*!
     * \brief Simulates receiving a single character via UART during unit testing
     */
    void cli_test_inject_char(char c);
    
    /*!
     * \brief Simulates typing a complete string into the CLI during unit testing
     */
    void cli_test_inject_string(const char *str);

    /*!
     * \brief Resets internal CLI buffer and event flags before each unit test
     */
    void cli_test_reset(void);

    /*!
     * \brief Returns the last telemetry mode requested through the CLI
     */
    enum telemetry_mode cli_test_get_last_mode(void);

    /*!
     * \brief Returns the number of times the random noise command was executed
     */
    int cli_test_get_noise_toggle_count(void);
    #endif


#endif 