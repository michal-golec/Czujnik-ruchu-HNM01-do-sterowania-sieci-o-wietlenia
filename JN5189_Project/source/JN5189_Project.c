#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_adc.h"
#include "fsl_ctimer.h"


//Def global
#define ADC_BASE ADC0
#define ADC_CHANNEL 2
#define TIMER_BASE CTIMER0

volatile uint32_t adcResultValue = 0;
volatile uint32_t adcResultValue_old = 0;
volatile uint32_t diffVal = 0;
volatile bool isNewDataReady = false; //flaga

//FLAGI DIAGNOSTYCZNE
volatile bool diagTimer = false;
volatile bool diagADC = false;


void ADC0_SEQA_IRQHandler(void) {
	ADC_ClearStatusFlags(ADC_BASE, kADC_ConvSeqAInterruptFlag);
	diagADC = true; // Zgłaszamy, że przerwanie ADC w ogóle ożyło

	adc_result_info_t adcResultInfoStruct;

    if (ADC_GetChannelConversionResult(ADC_BASE, ADC_CHANNEL, &adcResultInfoStruct)) {
        adcResultValue = adcResultInfoStruct.result;
        isNewDataReady = true;
    }
}

void CTIMER0_IRQHandler(void) {
    CTIMER_ClearStatusFlags(TIMER_BASE, kCTIMER_Match3Flag);

    diagTimer = true; // Zgłaszamy, że timer tyka

    ADC_DoSoftwareTriggerConvSeqA(ADC_BASE);
}

void Init_Hardware_Timer(void) {
    // Enable clock and power
    CLOCK_EnableClock(kCLOCK_Timer0);

    ctimer_config_t timerConfig;
    CTIMER_GetDefaultConfig(&timerConfig);
    CTIMER_Init(TIMER_BASE, &timerConfig);

    //Match channel config
    ctimer_match_config_t matchConfig;
    matchConfig.enableCounterReset = true;          // Reset counter on match
    matchConfig.enableCounterStop = false;
    matchConfig.matchValue = SystemCoreClock / 100; // Trigger threshold
    matchConfig.outControl = kCTIMER_Output_NoAction;
    matchConfig.outPinInitState = false;
    matchConfig.enableInterrupt = true;

    CTIMER_SetupMatch(TIMER_BASE, kCTIMER_Match_3, &matchConfig);
    CTIMER_StartTimer(TIMER_BASE);
}


int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();


    //config ADC
    CLOCK_EnableClock(kCLOCK_Adc0);
    POWER_EnablePD(kPDRUNCFG_PD_LDO_ADC_EN);
	PMC->PDRUNCFG |= PMC_PDRUNCFG_ENA_LDO_ADC_MASK;

	adc_config_t configuration;
	ADC_GetDefaultConfig(&configuration);
	configuration.clockDividerNumber = 7;
	ADC_Init(ADC_BASE, &configuration);

	//adc SeqA config
	adc_conv_seq_config_t adcConvSeqAConfigStruct = {0};
	adcConvSeqAConfigStruct.channelMask = (1U << ADC_CHANNEL);
	adcConvSeqAConfigStruct.triggerMask = 0U; //->software trigger
	adcConvSeqAConfigStruct.triggerPolarity = kADC_TriggerPolarityPositiveEdge;
	adcConvSeqAConfigStruct.interruptMode = kADC_InterruptForEachSequence;
	adcConvSeqAConfigStruct.enableSingleStep = false;
	adcConvSeqAConfigStruct.enableSyncBypass = false;

	//save & Seq Start
	ADC_SetConvSeqAConfig(ADC_BASE, &adcConvSeqAConfigStruct);
	ADC_EnableConvSeqA(ADC_BASE, true);
	ADC_EnableInterrupts(ADC_BASE, kADC_ConvSeqAInterruptEnable);

	EnableIRQ(ADC0_SEQA_IRQn);
	EnableIRQ(CTIMER0_IRQn);
	Init_Hardware_Timer();

	//Global interrupt enable
	__enable_irq();


    while(1) {
//    	// Diagnostyka Timera
//        if (diagTimer) {
//            PRINTF("TIMER ZYJE!\r\n");
//            diagTimer = false;
//        }
//
//        // Diagnostyka ADC
//        if (diagADC) {
//            PRINTF("ADC ZYJE!\r\n");
//            diagADC = false;
//        }

		if (isNewDataReady) {
			diffVal = adcResultValue - adcResultValue_old;
			PRINTF("%d\r\n", diffVal);

			isNewDataReady = false; //clearing flag
			adcResultValue_old = adcResultValue;
		}
	}
	return 0;
}
