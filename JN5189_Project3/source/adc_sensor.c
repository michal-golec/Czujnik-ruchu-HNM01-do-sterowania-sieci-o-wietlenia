#include "adc_sensor.h"

volatile uint32_t adcResultValue = 0;
volatile uint32_t maxValue = 0;
volatile uint32_t minValue = 0xFFFFFFFF;
volatile uint32_t filteredAdcValue = 0;
volatile bool isFirstSample = true;

//zmienne do timerów
volatile bool processDataFlag = false;


void ADC_Init_Custom(void) {
    PMC->PDRUNCFG |= PMC_PDRUNCFG_ENA_LDO_ADC_MASK;

    adc_config_t configuration;
    ADC_GetDefaultConfig(&configuration);
    configuration.clockMode = kADC_ClockSynchronousMode;
    configuration.clockDividerNumber = 15;
    ADC_Init(ADC_BASE, &configuration);

    adc_conv_seq_config_t adcConvSeqAConfigStruct = {0};
    adcConvSeqAConfigStruct.channelMask = (1U << ADC_CHANNEL);
    adcConvSeqAConfigStruct.triggerMask = 0U;
    adcConvSeqAConfigStruct.triggerPolarity = kADC_TriggerPolarityPositiveEdge;
    adcConvSeqAConfigStruct.interruptMode = kADC_InterruptForEachSequence;

    ADC_SetConvSeqAConfig(ADC_BASE, &adcConvSeqAConfigStruct);
    CLOCK_uDelay(300);

    ADC_EnableConvSeqA(ADC_BASE, true);
    ADC_EnableConvSeqABurstMode(ADC_BASE, true);
    ADC_EnableInterrupts(ADC_BASE, kADC_ConvSeqAInterruptEnable);
    EnableIRQ(ADC0_SEQA_IRQn);
}

void ADC0_SEQA_IRQHandler(void) {
    ADC_ClearStatusFlags(ADC_BASE, kADC_ConvSeqAInterruptFlag);

    adc_result_info_t adcResultInfoStruct;
    if (ADC_GetChannelConversionResult(ADC_BASE, ADC_CHANNEL, &adcResultInfoStruct)) {
        adcResultValue = adcResultInfoStruct.result;

        if (isFirstSample) {
            filteredAdcValue = adcResultValue;
            isFirstSample = false;
        } else {
            filteredAdcValue = ((filteredAdcValue * 63) + adcResultValue) >> 6;
        }

        if (filteredAdcValue > maxValue) maxValue = filteredAdcValue;
        if (filteredAdcValue < minValue) minValue = filteredAdcValue;
    }
}


void ADC_GetData_Frequence_Timeout(uint16_t DataFreq) {
	// ===================================================
	// TIMER 2: Sterowanie częstością przetwarzania ADC
	// ===================================================
	uint16_t processDataTimer = DataFreq;
	if (processDataTimer > 0) {
		processDataTimer--;
		if (processDataTimer == 0) {
			processDataFlag = true;                     // Wyzwalamy obliczenia w pętli while
			processDataTimer = DataFreq;     // Przeładowanie licznika (od nowa)
		}
	}
}
