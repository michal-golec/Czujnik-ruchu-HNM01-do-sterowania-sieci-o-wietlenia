// WERSJA Z ADC BURST MODE na wzór Michała Zbiecia
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_ctimer.h"

#include "adc_sensor.h"
#include "led_logic.h"

#define FADE_DIVIDER 30 // rozjaśnienie przez FADE_DIVIDER * 100ms
#define OFF_TO_ON_DELAY 100
#define PROCESS_INTERVAL_MS 10 // Czas w milisekundach, co ile pobieramy dane z ADC (np. 10 ms)
#define LED_TIMEOUT 2500

/*//////////////////////////////////////////////////////////////////////////////////

TODO: dobrać dobre filtry
Jeśli wybierzesz okno 1 ms lub 2 ms: Zostaw obecny wzór (stała 256)
noiseFloor = ((noiseFloor * 255) + finalVal) / 256;

Jeśli wybierzesz okno 4 ms lub 5 ms: Użyj stałej 64
noiseFloor = ((noiseFloor * 63) + finalVal) / 64;

Jeśli wybierzesz okno 10 ms: Użyj stałej 32
noiseFloor = ((noiseFloor * 31) + finalVal) / 32;

Jeśli wybierzesz okno 20 ms: Użyj stałej 16
noiseFloor = ((noiseFloor * 15) + finalVal) / 16;

///////////////////////////////////////////////////////////////////////////////////*/


volatile uint16_t fadeTime = FADE_DIVIDER;
volatile uint8_t targetPwmDuty = 10;
volatile uint16_t OffToOnDelay = OFF_TO_ON_DELAY;
volatile uint16_t DataFreq = PROCESS_INTERVAL_MS;
uint32_t finalVal = 0;
uint16_t ledOnTimeout = LED_TIMEOUT;


//przewanie do sterowania LED
void SysTick_Handler(void) {

	//Zmiana wypelnienia PWM
	// ===================================================
	// TIMER 4: Sterowanie czasem rozjaśnienia (zmiany wypełnienia PWM)
	// ===================================================
	LED_Fade_Timeout(targetPwmDuty, fadeTime);

	// ===================================================
	// TIMER 1: Sterowanie czasem świecenia LED
	// ===================================================
	LED_Process_Timeout(ledOnTimeout);


    // ===================================================
	// TIMER 2: Sterowanie częstością przetwarzania ADC
	// ===================================================
	ADC_GetData_Frequence_Timeout(DataFreq);

	// ===================================================
	// TIMER 3: Delay przed ponownym zapaleniem
	// ===================================================
	LED_StayOFF_Timeout(OffToOnDelay);

}





int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    //Podpięcie źródła zegara 32MHz do multiplexera ADC
    SYSCON->ADCCLKSEL = 0; // Wybór źródła zegara (0 = główny oscylator)
	SYSCON->ADCCLKDIV = 0; // Aktywacja dzielnika i wyłączenie pauzy (HALT)


	//config ADC
//	CLOCK_EnableClock(kCLOCK_Adc0);

//    adcConvSeqAConfigStruct.enableSingleStep = false;
//    adcConvSeqAConfigStruct.enableSyncBypass = false;

	PWM_Init_Custom();
	ADC_Init_Custom();


    SysTick_Config(SystemCoreClock / 1000);
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

			Process_Sensor_Data(finalVal);
    	}
		__WFI();
	}
    return 0;
}
