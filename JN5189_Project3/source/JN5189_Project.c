// WERSJA Z ADC BURST MODE na wzór Michała Zbiecia
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
#define SENSITIVITY_MARGIN 200
#define STARTUP_THRESHOLD 500	//próg przełączania
#define PROCESS_INTERVAL_MS 5 // Czas w milisekundach, co ile pobieramy dane z ADC (np. 5 ms)
#define OFF_TO_ON_DELAY 1000
/*//////////////////////////////////////////////////////////////////////////////////
TODO: dobrać mniejsze SENSITIVITY_MARGIN

TODO: dobrać dobre filtry
Jeśli wybierzesz okno 1 ms lub 2 ms: Zostaw obecny wzór (stała 256)
noiseFloor = ((noiseFloor * 255) + finalVal) / 256;

Jeśli wybierzesz okno 4 ms lub 5 ms: Użyj stałej 64
noiseFloor = ((noiseFloor * 63) + finalVal) / 64;

Jeśli wybierzesz okno 10 ms: Użyj stałej 32
noiseFloor = ((noiseFloor * 31) + finalVal) / 32;

Jeśli wybierzesz okno 20 ms: Użyj stałej 16
noiseFloor = ((noiseFloor * 15) + finalVal) / 16;

TODO: zapobiec mruganiu, delay przy zapalaniu ponownym po zgaszeniu
TODO: pwm
///////////////////////////////////////////////////////////////////////////////////*/







// zmienne globalne
volatile uint32_t adcResultValue = 0;
volatile uint32_t maxValue = 0;
volatile uint32_t minValue = 0xFFFFFFFF;
uint32_t finalVal = 0;
volatile uint32_t state = 0;

volatile uint16_t ledTimeoutMs = 0;	//licznik do LED
uint32_t printDelayCounter = 0;

uint32_t noiseFloor = STARTUP_THRESHOLD - SENSITIVITY_MARGIN;

//zmienne do filtru w przerwaniu adc
volatile uint32_t filteredAdcValue = 0;
volatile bool isFirstSample = true;

//zmienne do timerów
volatile uint16_t processDataTimer = PROCESS_INTERVAL_MS;
volatile bool processDataFlag = false;
volatile uint16_t OffDelayTimer = PROCESS_INTERVAL_MS;
volatile bool OffDelayFlag = false;

//przewanie do sterowania LED
void SysTick_Handler(void) {
	// ===================================================
	// TIMER 1: Sterowanie czasem świecenia LED
	// ===================================================
    if (ledTimeoutMs > 0) {
        ledTimeoutMs--;
        if (ledTimeoutMs == 0) {
            GPIO_PinWrite(GPIO, LED_PORT, LED_PIN, 0); //zgaszenie LED po upływie LED_TIMEOUT
            state = 0;
        }
    }


    // ===================================================
	// TIMER 2: Sterowanie częstością przetwarzania ADC
	// ===================================================
	if (processDataTimer > 0) {
		processDataTimer--;
		if (processDataTimer == 0) {
			processDataFlag = true;                     // Wyzwalamy obliczenia w pętli while
			processDataTimer = PROCESS_INTERVAL_MS;     // Przeładowanie licznika (od nowa)
		}
	}

	// ===================================================
	// TIMER 3: Delay przed ponownym zapaleniem
	// ===================================================
	if (OffDelayTimer > 0) {
		OffDelayTimer--;
		if (OffDelayTimer == 0) {
			OffDelayFlag = true;
			}
	}

}


//przerwanie ADC
void ADC0_SEQA_IRQHandler(void) {

    ADC_ClearStatusFlags(ADC_BASE, kADC_ConvSeqAInterruptFlag);

    adc_result_info_t adcResultInfoStruct;
    if (ADC_GetChannelConversionResult(ADC_BASE, ADC_CHANNEL, &adcResultInfoStruct)) {

    	adcResultValue = adcResultInfoStruct.result;

    	// Inicjalizacja filtru pierwszą próbką z ADC
		if (isFirstSample) {
			filteredAdcValue = adcResultValue;
			isFirstSample = false;
		} else {
			// Sprzętowy filtr dolnoprzepustowy (EMA ze stałą K=8).
			// Zapis >> 3 to przesunięcie bitowe, działające jak dzielenie przez 8,
			// ale wielokrotnie szybsze dla procesora.
			filteredAdcValue = ((filteredAdcValue * 7) + adcResultValue) >> 3;
		}


    	if (filteredAdcValue > maxValue){
			maxValue = filteredAdcValue;
		}
		if (filteredAdcValue < minValue){
			minValue = filteredAdcValue;
		}
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

    	if (processDataFlag) {
    		processDataFlag = false;

    		__disable_irq();
			uint32_t localMax = maxValue;
			uint32_t localMin = minValue;

			maxValue = 0;
			minValue = 0xFFFFFFFF;
			__enable_irq();

			if (localMax >= localMin) {
				finalVal = localMax - localMin;
			} else {
				finalVal = 0;
			}

//////////////////////////////////////////////////////////////////////////
			// Algorytm obliczania szumu gdy nie wykryto ruchu


			uint32_t dynamicThreshold = noiseFloor + SENSITIVITY_MARGIN;

			if (state == 0) {
				// STAN CISZY
				noiseFloor = ((noiseFloor * 63) + finalVal) / 64;

				if (finalVal > dynamicThreshold && OffDelayFlag) {
					GPIO_PinWrite(GPIO, LED_PORT, LED_PIN, 1);
					state = 1;
					OffDelayTimer = OFF_TO_ON_DELAY;
					ledTimeoutMs = LED_TIMEOUT;
				}
			}
			//TODO: tego else chyba usunąć, bo jest bez sensu, koliduje z timerem offdelay
			else {
				// STAN RUCHU
				if (finalVal > dynamicThreshold) {
					ledTimeoutMs = LED_TIMEOUT;
				}
			}


////////////////////////////////////////////////////////////////

			printDelayCounter++;
			if (printDelayCounter >= 0) { // print co 10 000 probek
				PRINTF("Max: %u | Min: %u | Szum: %u | Roznica: %u | Prog: %u | Stan: %u\r\n", localMax, localMin, noiseFloor, finalVal, dynamicThreshold, state);
				printDelayCounter = 0;
			}
		}
		__WFI();
	}
    return 0;
}
