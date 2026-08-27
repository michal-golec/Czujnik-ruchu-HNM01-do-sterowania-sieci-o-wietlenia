// WERSJA Z ADC BURST MODE
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_adc.h"
#include <stdlib.h>


#define ADC_BASE ADC0
#define ADC_CHANNEL 2
#define BUF_SIZE 64 // Większy bufor dla bardzo szybkiego strumienia danych

// --- ZMIENNE GLOBALNE ---
volatile uint32_t adcResultValue = 0;
volatile bool isNewDataReady = false; //flaga
volatile uint16_t adcBuffer[BUF_SIZE] = {0};
volatile uint8_t bufIndex = 0;
volatile uint32_t conversionCount = 0; // Licznik kontrolny dla terminala
volatile uint32_t runningSum = 0;            // Suma krocząca
volatile uint16_t currentMean = 0;           // Wartosc tla
volatile uint32_t finalVal = 0;


/* --- PRZERWANIE ADC (Praca Ciągła) --- */
void ADC0_SEQA_IRQHandler(void) {
    // 1. Czyszczenie flagi
    ADC_ClearStatusFlags(ADC_BASE, kADC_ConvSeqAInterruptFlag);

    // 2. Pobranie i zapis wyniku
    adc_result_info_t adcResultInfoStruct;
    if (ADC_GetChannelConversionResult(ADC_BASE, ADC_CHANNEL, &adcResultInfoStruct)) {

    	adcResultValue = adcResultInfoStruct.result;
    	isNewDataReady = true;
    }

}

/* --- FUNKCJA GŁÓWNA --- */
int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    // 1. Podpięcie źródła zegara 32MHz do multiplexera ADC
    SYSCON->ADCCLKSEL = 0; // Wybór źródła zegara (0 = główny oscylator)
	SYSCON->ADCCLKDIV = 0; // Aktywacja dzielnika i wyłączenie pauzy (HALT)

	// --- 2. WŁĄCZENIE ZEGARA CYFROWEGO (Tego brakowało!) ---
	CLOCK_EnableClock(kCLOCK_Adc0);
    POWER_EnablePD(kPDRUNCFG_PD_LDO_ADC_EN);
    PMC->PDRUNCFG |= PMC_PDRUNCFG_ENA_LDO_ADC_MASK;

    adc_config_t configuration;
    ADC_GetDefaultConfig(&configuration);
    configuration.clockMode = kADC_ClockSynchronousMode;   // <-- DODANE
    configuration.clockDividerNumber = 15;	// Ustawiamy najszybszy dzielnik zegara dla ADC (0 = max speed)
    ADC_Init(ADC_BASE, &configuration);

    adc_conv_seq_config_t adcConvSeqAConfigStruct = {0};
    adcConvSeqAConfigStruct.channelMask = (1U << ADC_CHANNEL);
    adcConvSeqAConfigStruct.triggerMask = 0U;
    adcConvSeqAConfigStruct.triggerPolarity = kADC_TriggerPolarityPositiveEdge;
    adcConvSeqAConfigStruct.interruptMode = kADC_InterruptForEachSequence;
    adcConvSeqAConfigStruct.enableSingleStep = false;
    adcConvSeqAConfigStruct.enableSyncBypass = false;

    ADC_SetConvSeqAConfig(ADC_BASE, &adcConvSeqAConfigStruct);

    CLOCK_uDelay(300);   //wymagane na JN5189 po konfiguracji sekwencji

    ADC_EnableConvSeqA(ADC_BASE, true);
    // Włączenie continuous
	ADC_EnableConvSeqABurstMode(ADC_BASE, true);
    ADC_EnableInterrupts(ADC_BASE, kADC_ConvSeqAInterruptEnable);


    EnableIRQ(ADC0_SEQA_IRQn);
    __enable_irq();


    while(1) {

    	if (isNewDataReady) {
			//Aktualizacja tla
			runningSum -= adcBuffer[bufIndex];      // Usun najstarsza
			adcBuffer[bufIndex] = adcResultValue;   // Zapisz najnowsza
			runningSum += adcBuffer[bufIndex];      // Dodaj najnowszą do sumy

			currentMean = runningSum / BUF_SIZE;    // Wyliczanie tla (sr. aryt.)

			//wspolczynnik mad (odchylenie bezwzgledne)
			uint32_t madSum = 0; //zmienna pomoc

			for (int i = 0; i < BUF_SIZE; i++) {
				madSum += abs(adcBuffer[i] - currentMean);
			}

			finalVal = madSum/BUF_SIZE;

			//inkrementacja indeksu bufora (reszta z dzielenia przez BUF_SIZE)
			bufIndex = (bufIndex + 1) % BUF_SIZE;


			PRINTF("%u\r\n", finalVal);

			//Sprawdzenie czy ruch przekroczyl prog
//			if (finalVal > MOVEMENT_THRESHOLD) {
//				GPIO_PinWrite(GPIO, LED_PORT, LED_PIN, 1);
//				ledTimeoutCounter = LED_TIME_TICKS;
//			}

			//zgadzenie diody
//			if (ledTimeoutCounter > 0) {
//				ledTimeoutCounter--; 	//Odejmuje 1 co 10 ms
//
//				if (ledTimeoutCounter == 0) {
//					GPIO_PinWrite(GPIO, LED_PORT, LED_PIN, 0);
//				}
//			}

			isNewDataReady = false;
		}
    }
    return 0;
}
