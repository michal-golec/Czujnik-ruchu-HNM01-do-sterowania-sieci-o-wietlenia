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
#define LED_PORT 0
#define LED_PIN 19
#define LED_TIMEOUT 2000    // 2000ms
#define SENSITIVITY_MARGIN 50

// zmienne globalne
volatile uint32_t adcResultValue = 0;
volatile bool isNewDataReady = false; //flaga
volatile uint16_t adcBuffer[BUF_SIZE] = {0};
uint8_t bufIndex = 0;
//volatile uint32_t conversionCount = 0; // Licznik kontrolny dla terminala
uint32_t runningSum = 0;            // Suma krocząca
uint16_t currentMean = 0;           // Wartosc tla
uint32_t noiseFloor = 0;		// Wartosc tla po filtracji
uint32_t noiseFloorAccumulator = 0;
uint32_t finalVal = 0;
volatile uint32_t state = 0;

volatile uint16_t ledTimeoutMs = 0;	//licznik do LED
uint32_t printDelayCounter = 0;


//przewanie do sterowania LED
void SysTick_Handler(void) {
    if (ledTimeoutMs > 0) {
        ledTimeoutMs--;
        if (ledTimeoutMs == 0) {
            GPIO_PinWrite(GPIO, LED_PORT, LED_PIN, 0); //zgaszenie LED po upływie LED_TIMEOUT
            state = 0;
        }
    }
}


//przerwanie ADC
void ADC0_SEQA_IRQHandler(void) {

    ADC_ClearStatusFlags(ADC_BASE, kADC_ConvSeqAInterruptFlag);

    adc_result_info_t adcResultInfoStruct;
    if (ADC_GetChannelConversionResult(ADC_BASE, ADC_CHANNEL, &adcResultInfoStruct)) {

    	adcResultValue = adcResultInfoStruct.result;
    	isNewDataReady = true;
    }

}


int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    //Podpięcie źródła zegara 32MHz do multiplexera ADC
    SYSCON->ADCCLKSEL = 0; // Wybór źródła zegara (0 = główny oscylator)
	SYSCON->ADCCLKDIV = 0; // Aktywacja dzielnika i wyłączenie pauzy (HALT)

	//config LED
	CLOCK_EnableClock(kCLOCK_Gpio0);
	gpio_pin_config_t led_config = {kGPIO_DigitalOutput, 0};
	GPIO_PinInit(GPIO, LED_PORT, LED_PIN, &led_config);

	//config ADC
	CLOCK_EnableClock(kCLOCK_Adc0);
//    POWER_EnablePD(kPDRUNCFG_PD_LDO_ADC_EN);
    PMC->PDRUNCFG |= PMC_PDRUNCFG_ENA_LDO_ADC_MASK;

    adc_config_t configuration;
    ADC_GetDefaultConfig(&configuration);
    configuration.clockMode = kADC_ClockSynchronousMode;
    configuration.clockDividerNumber = 15;	//dzielnik zegara dla ADC (0 = max speed)
    ADC_Init(ADC_BASE, &configuration);

    adc_conv_seq_config_t adcConvSeqAConfigStruct = {0};
    adcConvSeqAConfigStruct.channelMask = (1U << ADC_CHANNEL);
    adcConvSeqAConfigStruct.triggerMask = 0U;
    adcConvSeqAConfigStruct.triggerPolarity = kADC_TriggerPolarityPositiveEdge;
    adcConvSeqAConfigStruct.interruptMode = kADC_InterruptForEachSequence;
    adcConvSeqAConfigStruct.enableSingleStep = false;
    adcConvSeqAConfigStruct.enableSyncBypass = false;

    ADC_SetConvSeqAConfig(ADC_BASE, &adcConvSeqAConfigStruct);

    CLOCK_uDelay(300);   //PODOBNO wymagane na JN5189 po konfiguracji sekwencji

    SysTick_Config(SystemCoreClock / 1000);


    ADC_EnableConvSeqA(ADC_BASE, true);
	ADC_EnableConvSeqABurstMode(ADC_BASE, true);	// Włączenie continuous
    ADC_EnableInterrupts(ADC_BASE, kADC_ConvSeqAInterruptEnable);
    EnableIRQ(ADC0_SEQA_IRQn);
    __enable_irq();


    while(1) {

    	if (isNewDataReady) {

			// Filtry IIR

			if (currentMean == 0) currentMean = adcResultValue;
			// Wygładzanie tła (odpowiednik bufora)
			currentMean = ((currentMean * 63) + adcResultValue) / 64;

			// Obliczenie odchylenia
			uint32_t deviation = abs((int32_t)adcResultValue - (int32_t)currentMean);

			// Wygładzanie sygnału ruchu
			finalVal = ((finalVal * 7) + deviation) / 8;

			if (ledTimeoutMs == 0) {
//				noiseFloor = ((noiseFloor * 255) + finalVal) / 256;
//			}
				//dodanie akumulatora jako bufor z czescia ulamkowa
				if (noiseFloorAccumulator == 0) {
					noiseFloorAccumulator = finalVal * 256;
				}
				else {
					noiseFloorAccumulator = noiseFloorAccumulator - (noiseFloorAccumulator / 256) + finalVal;
				}

				// Obcięcie cz. ulamkowej na potrzeby reszty programu
				noiseFloor = noiseFloorAccumulator / 256;
			}

			uint32_t dynamicThreshold = noiseFloor + SENSITIVITY_MARGIN;

			//obsługa LED, przerwanie adc -> ON, przerwanie systick -> OFF
			if (finalVal > dynamicThreshold) {
				GPIO_PinWrite(GPIO, LED_PORT, LED_PIN, 1);
				state = 1;
				ledTimeoutMs = LED_TIMEOUT; // delay na 2000 ms
			}


			printDelayCounter++;
			if (printDelayCounter >= 10000) { // print co 10 000 probek
				PRINTF("Sygnal: %u | Szum: %u | Prog: %u | Stan: %u\r\n", finalVal, noiseFloor, dynamicThreshold, state);
				printDelayCounter = 0;
			}

			isNewDataReady = false;
		}
		__WFI();
	}
    return 0;
}
