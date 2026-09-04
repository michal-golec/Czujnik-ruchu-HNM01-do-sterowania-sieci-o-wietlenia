#ifndef ADC_SENSOR_H
#define ADC_SENSOR_H

#include "fsl_adc.h"

#define ADC_BASE ADC0
#define ADC_CHANNEL 2

extern volatile uint32_t maxValue;
extern volatile uint32_t minValue;
extern volatile bool processDataFlag;
//extern volatile uint32_t filteredAdcValue;

void ADC_Init_Custom(void);
void ADC0_SEQA_IRQHandler(void);
void ADC_GetData_Frequence_Timeout(uint16_t DataFreq);

#endif
