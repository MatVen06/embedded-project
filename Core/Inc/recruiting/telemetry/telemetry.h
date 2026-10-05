#ifndef TELEMETRY_H
#define TELEMETRY_H

    #include<stdint.h>
    #define TELEMETRY_ADC_BUFFER_SIZE 2

    void telemetry_init(void); 
    // initializes data stream by starting the adc in DMA circular mode to read 
    //potentiometer and thermoresistor
    void telemetry_update(void);
    //Uses non blocking timer to format raw sensor data and transmit it over UART every 100ms 



#endif