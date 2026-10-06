#ifndef BUTTON_H
#define BUTTON_H

    #include<stdbool.h>
    #include<stdint.h>
    /*!
     * \brief scans for button presses
     */
    void button_state_update(void);

    /*!
     * \brief returns true if its in CLI mode, pauses telemetry stream
     */
    bool button_cli_mode(void);


#endif