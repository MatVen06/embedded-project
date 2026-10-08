#ifndef BUTTON_H
#define BUTTON_H

    #include<stdbool.h>
    #include<stdint.h>
    /*!
     * \brief scans for button presses
     */
    void button_state_update(void);

    /*!
     * \brief detects whether the button was pressed or not
     */
    bool button_was_pressed(void);


#endif