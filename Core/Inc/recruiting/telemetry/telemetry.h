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
    #define TELEMETRY_WINDOW_SIZE 150

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

    /*!
     * \brief converts the raw pot values to degrees celsius
     * \param raw_adc Raw 12-bit ADC value (0 - 4095).
     * \return Temperature in degrees Celsius.
     */
    float telemetry_pot_to_celsius(uint16_t raw_adc);

    /*!
     * \brief converts the ntcs pot values to degrees celsius
     * \param raw_adc Raw 12-bit ADC value (0 - 4095).
     * \return Temperature in degrees Celsius.
     */
    float telemetry_ntc_to_celsius(uint16_t raw_adc);


    /*!
     * \brief Resets the moving average circular buffers, sample index, and full flag.
     */    
    void telemetry_filter_reset(void);

    /*!
    * \brief Inserts new potentiometer and thermistor samples into the moving average circular buffers.
    * \param pot_sample Potentiometer raw sample to append.
    * \param ntc_sample NTC raw sample to append.
    */
    void telemetry_filter_add_sample(uint16_t pot_sample, uint16_t ntc_sample);

    /*!
    * \brief Calculates and returns the current moving average of the potentiometer.
    * \return Unsigned 16-bit moving average ADC count.
    */
    uint16_t telemetry_filter_get_pot_avg(void);

    /*!
    * \brief Calculates and returns the current moving average of the NTC thermistor.
    * \return Unsigned 16-bit moving average ADC count.
    */
    uint16_t telemetry_filter_get_ntc_avg(void);
    #ifdef UNIT_TEST
        /*!
         * \brief Injects simulated raw ADC readings for testing POST and differential logic
         */
        void telemetry_test_set_raw_adc(uint16_t pot, uint16_t ntc);
    #endif

#endif