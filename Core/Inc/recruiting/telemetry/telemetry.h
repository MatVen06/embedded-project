#ifndef TELEMETRY_H
#define TELEMETRY_H
    
    #include<stdint.h>
    #include<stdbool.h>
    #include <stdlib.h>


    /*!
     * \brief macro for the size of the array reading ntc and pot
     */
    #define TELEMETRY_ADC_BUFFER_SIZE 2

    /*!
     * \brief macro for the number of values kept in the moving average
     */ 
    #define TELEMETRY_WINDOW_SIZE 100

    /*!
     * \brief macro for the value of the noise amplitude
     */
    #define NOISE_AMPLITUDE 150

    /*!
     * \brief enum for the mutually exclusive modes of reading data
     */
    enum telemetry_mode
    {
        RAW,
        AVG
    };

    /*!
     * \brief initializes the data stream by starting the ADC in DMA circular mode to read the potentiometer and thermistor.
    */
    void telemetry_init(void); 

    /*!
     * \brief Uses non blocking timer to format raw sensor data and transmit it over UART every 100ms 
     */
    void telemetry_update(void);

    /*!
     * \brief setter for the various modes of the CLI
     */
    void telemetry_set_mode(enum telemetry_mode new_mode);

    /*!
     * \brief toggles the noise filter
     */

    void telemetry_noise_toggle(void);

    /*!
     * \brief getter for the last raw pot value (necessary for the fsm)
     */
    uint16_t telemetry_get_raw_pot(void);

    /*!
     * \brief getter for the last raw ntc value (necessary for the fsm)
     */
    uint16_t telemetry_get_raw_ntc(void);


#endif