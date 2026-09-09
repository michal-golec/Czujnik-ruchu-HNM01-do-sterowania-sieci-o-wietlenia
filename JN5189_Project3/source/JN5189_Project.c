// WERSJA Z ADC BURST MODE na wzór Michała Zbiecia
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_ctimer.h"

#include "adc_sensor.h"
#include "led_logic.h"

#define FADE_TIME 3000 // rozjaśnienie [ms]
#define OFF_TO_ON_DELAY 100
#define PROCESS_INTERVAL_MS 100 // Czas w milisekundach, co ile pobieramy dane z ADC (np. 10 ms)
#define LED_ON_TIMEOUT_MS 5000 // Czas świecenie [ms]
#define LED_STANDBY_TIMEOUT_MS 5000 // Czas po którym przechodzi w MODE_STANDBY, gdy nie ma ruchu [ms] (1800000ms = 30min)
#define SENSITIVITY_MARGIN 80
#define SENSITIVITY_LEVEL_SIZE 5


// Nowe stałe do analizy obwiedni
#define ENVELOPE_DECAY_RATE 15  // Szybkość opadania obwiedni (im wyższa, tym szybciej spada)
#define TREND_UP_MARGIN 20
#define TREND_DOWN_MARGIN 5

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


volatile uint16_t fadeTime = FADE_TIME;
volatile uint8_t minDuty = 10;
volatile uint8_t maxDuty = 100;
volatile uint16_t OffToOnDelay = OFF_TO_ON_DELAY;
volatile uint16_t DataFreq = PROCESS_INTERVAL_MS;
uint32_t finalVal = 0;
uint32_t sensitivity = SENSITIVITY_MARGIN;
uint32_t sensitivityLevel[SENSITIVITY_LEVEL_SIZE] = {
		40, 70, SENSITIVITY_MARGIN, 100, 120
};

volatile uint16_t ledOnTimeout = LED_ON_TIMEOUT_MS;
volatile uint32_t ledOffTimeout = LED_STANDBY_TIMEOUT_MS;

typedef enum {
    MODE_NORMAL,
    MODE_STANDBY,
	MODE_CALIBRATION
} SystemMode_t;

SystemMode_t currentMode = MODE_NORMAL;

//zmienne do kalibracji
uint32_t senCalibTimeStep = 3000; //[ms]
uint32_t senCalibStep = 5;
bool senCalibEnable = false;

//przewanie do sterowania LED
void SysTick_Handler(void) {

	//kalibracja sensitivity
	Sensitivity_Calibration_Timer(senCalibEnable, senCalibTimeStep, senCalibStep);

	/////////////////////////////////////////////////////////////


	// Timer do standby
	if (ledState == 0){
		if (ledOffTimeout > 0) ledOffTimeout--;
	}
	else {
		ledOffTimeout = LED_STANDBY_TIMEOUT_MS;
	}

	// ===================================================
	// TIMER 2: Sterowanie częstością przetwarzania ADC
	// ===================================================
	ADC_GetData_Frequence_Timeout(DataFreq);

	// ===================================================
	// TIMER 3: Delay przed ponownym zapaleniem
	// ===================================================
	LED_StayOFF_Timeout(OffToOnDelay);


	//Zmiana wypelnienia PWM
	// ===================================================
	// TIMER 4: Sterowanie czasem rozjaśnienia (zmiany wypełnienia PWM)
	// ===================================================
	LED_Process_Fade(minDuty, maxDuty, fadeTime);

	// ===================================================
	// TIMER 1: Sterowanie czasem świecenia LED
	// ===================================================
	LED_Process_Timeout(ledOnTimeout);






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


    SysTick_Config(SystemCoreClock / 1000);	//systick bije co 1ms
    __enable_irq();


    CLOCK_uDelay(2000000);
    PRINTF("/////////////////////\r\nStart petli glownej\r\n/////////////////////\r\n");
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



			// ===================================================
			// 1. DETEKTOR OBWIEDNI (PEAK DETECTOR)
			// ===================================================
			static uint32_t envelope = 0;
			if (finalVal > envelope) {
				envelope = finalVal; // Błyskawiczny wzrost do szczytu fali
			} else {
				if (envelope > ENVELOPE_DECAY_RATE) {
					envelope -= ENVELOPE_DECAY_RATE; // Łagodne opadanie
				} else {
					envelope = 0;
				}
			}

			// ===================================================
			// 2. FILTR DOLNOPRZEPUSTOWY OBWIEDNI (WYGŁADZANIE)
			// ===================================================
			static uint32_t filteredEnvelope = 0;
			filteredEnvelope = ((filteredEnvelope * 7) + envelope) >> 3;

			// ===================================================
			// 3. DETEKCJA ZBLIŻANIA / ODDALANIA (TREND)
			// ===================================================
			static uint32_t fastAvg = 0;
			static uint32_t slowAvg = 0;
			int8_t trend = 0; // 1 = Zbliżanie, -1 = Oddalanie, 0 = Stabilnie

			// fastAvg reaguje dynamicznie (okno ~4 próbki), slowAvg stanowi bazę (okno ~16 próbek)
			fastAvg = ((fastAvg * 3) + filteredEnvelope) >> 2;
			slowAvg = ((slowAvg * 15) + filteredEnvelope) >> 4;

			if (fastAvg > (slowAvg + TREND_UP_MARGIN)) {
				trend = 1;  // Zbliżanie
			} else if (fastAvg < (slowAvg - TREND_DOWN_MARGIN)) {
				trend = 2; // Oddalanie
			}



			Process_Sensor_Data(finalVal, sensitivity, trend);

			PRINTF("fastAvg = %u | slowAvg = %u | trend = %d | stan = %u\r\n",
						fastAvg, slowAvg, trend, ledState);



			// ===================================================
			// MASZYNA STANÓW SYSTEMU
			// ===================================================
			switch (currentMode) {
				case MODE_NORMAL:
					minDuty = 10;
					maxDuty = 100;
					sensitivity = sensitivityLevel[2];

					if (ledOffTimeout == 0) {
						currentMode = MODE_STANDBY;

//						PRINTF("Brak ruchu przez 30 minut. Przejscie w STANDBY (0%%).\r\n");
					}
					else if (senCalibEnable){
						PRINTF("Kalibracja START!\r\n");
						senCalibEnable = true;
						currentMode = MODE_CALIBRATION;
					}
					break;

				case MODE_STANDBY:
					minDuty = 0;
					maxDuty = 10;
					sensitivity = sensitivityLevel[1];

					if (ledState == 1) {
						currentMode = MODE_NORMAL;
//						PRINTF("Wykryto ruch! Powrot do NORMAL (10%%-100%%).\r\n");
					}
					break;

				case MODE_CALIBRATION:
					minDuty = 0;
					maxDuty = 100;
					PRINTF("Calib...\r\n");
					if (ledState == 1){
						senCalibEnable = false;
						PRINTF("Kalibracja zakonczona!\r\n");
						PRINTF("Ustalone poziomy czulosci:\r\n");
						for (int i = 0; i < SENSITIVITY_LEVEL_SIZE; i++){
							sensitivityLevel[i] = sensitivity + senCalibStep; //najniższy sensitivityLevel to ostatni przed zapaleniem
							sensitivity = sensitivity + 10;
							PRINTF("%u, ", sensitivityLevel[i]);
						}
						PRINTF("\r\n");
						CLOCK_uDelay(4000000);
						currentMode = MODE_NORMAL;
					}



			}
    	}
		__WFI();
	}
    return 0;
}
