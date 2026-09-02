#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_adc.h"
#include "fsl_ctimer.h"
#include <stdlib.h>


//Def global
#define ADC_BASE ADC0
#define ADC_CHANNEL 2
#define TIMER_BASE CTIMER0
#define BUF_SIZE 16
//#define MOVEMENT_THRESHOLD 100 // PRÓG TODO: Aktywna zmiana wzgledem tla
#define LED_PORT 0
#define LED_PIN 19
#define LED_TIME_TICKS 200    // 200 * 10ms = 2 s
#define SENSITIVITY_MARGIN 60

volatile uint32_t adcResultValue = 0;
volatile bool isNewDataReady = false; //flaga

volatile uint16_t adcBuffer[BUF_SIZE] = {0}; // Pamięć
uint8_t bufIndex = 0;               // Wskaznik bufora
uint32_t runningSum = 0;            // Suma krocząca
uint16_t currentMean = 0;           // Wartosc tla* (prosta)
uint32_t noiseFloor = 0;		// Wartosc tla po filtracji
uint32_t finalVal = 0;
uint32_t state = 0;

volatile uint16_t ledTimeoutCounter = 0;	//licznik do LED

//FLAGI DIAGNOSTYCZNE
volatile bool diagTimer = false;
volatile bool diagADC = false;


void ADC0_SEQA_IRQHandler(void) {
	ADC_ClearStatusFlags(ADC_BASE, kADC_ConvSeqAInterruptFlag);
	diagADC = true; // Zglaszanie ze przerwanie ADC dziala

	adc_result_info_t adcResultInfoStruct;

    if (ADC_GetChannelConversionResult(ADC_BASE, ADC_CHANNEL, &adcResultInfoStruct)) {
        adcResultValue = adcResultInfoStruct.result;
        isNewDataReady = true;
    }
}

void CTIMER0_IRQHandler(void) {
    CTIMER_ClearStatusFlags(TIMER_BASE, kCTIMER_Match3Flag);

    diagTimer = true; // Zglaszanie ze timer tyka

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


    //config LED
    CLOCK_EnableClock(kCLOCK_Gpio0);
	gpio_pin_config_t led_config = {kGPIO_DigitalOutput, 0};
	GPIO_PinInit(GPIO, LED_PORT, LED_PIN, &led_config);


    //config ADC
    CLOCK_EnableClock(kCLOCK_Adc0);
//    POWER_DisablePD(kPDRUNCFG_PD_LDO_ADC_EN);
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
    	if (isNewDataReady) {
			//Aktualizacja tla
			runningSum -= adcBuffer[bufIndex];      // Usun najstarsza
			adcBuffer[bufIndex] = adcResultValue;   // Zapisz najnowsza
			runningSum += adcBuffer[bufIndex];      // Dodaj najnowszą do sumy

			currentMean = runningSum / BUF_SIZE;    // Wyliczanie tla (śr. arytmetyczna)

			//wspolczynnik mad (odchylenie bezwzgledne)
			uint32_t madSum = 0; //zmienna pomoc

			for (int i = 0; i < BUF_SIZE; i++) {
				madSum += abs((int32_t)adcBuffer[i] - (int32_t)currentMean);	//zmiana typow zeby roznica sie zgadzala
			}

			finalVal = madSum/BUF_SIZE;

			//inkrementacja indeksu bufora (reszta z dzielenia przez BUF_SIZE)
			bufIndex = (bufIndex + 1) % BUF_SIZE;



			//obliczanie szumu tla
			if (ledTimeoutCounter == 0) {
				// softwarowy filr dolnoprzepustowy
				noiseFloor = ((noiseFloor * 15) + finalVal) / 16;
			}

			//wyliczanie dynamicznego progu
			uint32_t dynamicThreshold = noiseFloor + SENSITIVITY_MARGIN;

			PRINTF("Sygnal: %u | Szum: %u | Prog: %u | Stan: %u\r\n", finalVal, noiseFloor, dynamicThreshold, state);

			if (finalVal > dynamicThreshold) {
				//wykryto ruch -> włączenie LED, zatrzymujemy obliczanie szumu
				GPIO_PinWrite(GPIO, LED_PORT, LED_PIN, 1);
				state = 1;
				ledTimeoutCounter = LED_TIME_TICKS;
			}

			if (ledTimeoutCounter > 0) {
				ledTimeoutCounter--;

				if (ledTimeoutCounter == 0 && finalVal < dynamicThreshold) {
					//minął delay -> gasimy LED, algorytm znów oblicza szum
					GPIO_PinWrite(GPIO, LED_PORT, LED_PIN, 0);
					state = 0;
				}
			}

			isNewDataReady = false;
		}
    	__WFI(); //uspienie az do przerwania CTIMER

	}
	return 0;
}
