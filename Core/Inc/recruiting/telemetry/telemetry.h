#ifndef TELEMETRY_H
#define TELEMETRY_H
    
    #include<stdint.h>
    #define TELEMETRY_ADC_BUFFER_SIZE 2

/*!
 * \brief 
 * initializes the data stream by starting the ADC in DMA circular mode to read the potentiometer and thermistor.
*/
    void telemetry_init(void); 


    /*!
     * \brief 
     * Uses non blocking timer to format raw sensor data and transmit it over UART every 100ms 

    */
    void telemetry_update(void);




#endif