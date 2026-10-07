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


#endif 