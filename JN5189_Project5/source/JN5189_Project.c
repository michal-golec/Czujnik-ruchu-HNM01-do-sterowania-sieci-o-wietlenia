// WERSJA Z ADC BURST MODE na wzór Michała Zbiecia
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"


#include "adc_sensor.h"
#include "led_logic.h"
#include "IR_Process.h"
#include "sen_Calib.h"

#define FADE_TIME 3000 // rozjaśnienie [ms]
#define OFF_TO_ON_DELAY 100
#define PROCESS_INTERVAL_MS 100 // Czas w milisekundach, co ile pobieramy dane z ADC (np. 10 ms)
#define LED_ON_TIMEOUT_MS 5000 // Czas świecenie [ms]
#define LED_STANDBY_TIMEOUT_MS 1800000 // Czas po którym przechodzi w MODE_STANDBY, gdy nie ma ruchu [ms] (1800000ms = 30min)
#define SENSITIVITY_MARGIN 30


// Nowe stałe do analizy obwiedni
#define ENVELOPE_DECAY_RATE 2  // Szybkość opadania obwiedni (im wyższa, tym szybciej spada)
#define TREND_UP_MARGIN 30
#define TREND_DOWN_MARGIN 30
#define UART_PRINT_TIMEOUT 5
#define FILTR2_DOWNSTEP 5

////


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

volatile uint16_t OffToOnDelay = OFF_TO_ON_DELAY;
volatile uint16_t DataFreq = PROCESS_INTERVAL_MS;
uint32_t finalVal = 0;
uint32_t Val = 0;
uint32_t Val2 = 0;
uint32_t filtr2DownStep = FILTR2_DOWNSTEP;
uint32_t filtr2FallCouter = 0;


typedef enum {
    MODE_NORMAL,
    MODE_STANDBY,
	MODE_CALIBRATION
} SystemMode_t;

SystemMode_t currentMode = MODE_NORMAL;

//zmienne do kalibracji
volatile uint32_t senCalibTimeout_ms = 20000;

uint32_t printDelayCounter = UART_PRINT_TIMEOUT;

SystemSettings_t sysSettings = {
	.ledStandByTimeout_ms = LED_STANDBY_TIMEOUT_MS,
	.ledOnTimeout_ms = LED_ON_TIMEOUT_MS,
	.currentColorTemp = 50,
	.minDuty = 30,
	.maxDuty = 100,
	.fadeTime_ms = 1500,
	.senCalibEnable = false,	////////////////////////tu ten wskaznik moze sie klucic ze struktura
	.standByModeEnable = true,
	.sensitivity = SENSITIVITY_MARGIN,
	.sensitivityLevel = {25, SENSITIVITY_MARGIN, 35, 40, 45}
};


//przewanie do sterowania LED
void SysTick_Handler(void) {

	//kalibracja sensitivity
	Sensitivity_Calibration_Timer(senCalibTimeout_ms);

	/////////////////////////////////////////////////////////////


	// ===================================================
	// TIMER 5: Czas bezruchu, po którym wchodzi w MODE_STANDBY
	// ===================================================
	LED_ToStandBy_Timeout(sysSettings.ledStandByTimeout_ms);

	// ===================================================
	// TIMER 2: Sterowanie częstością przetwarzania ADC
	// ===================================================
	ADC_GetData_Frequence_Timeout(DataFreq);



	//Zmiana wypelnienia PWM
	// ===================================================
	// TIMER 4: Sterowanie czasem rozjaśnienia (zmiany wypełnienia PWM)
	// ===================================================
	LED_Process_Fade(sysSettings.minDuty, sysSettings.maxDuty, sysSettings.fadeTime_ms);

	// ===================================================
	// TIMER 1: Sterowanie czasem świecenia LED
	// ===================================================
	LED_Process_Timeout(sysSettings.ledOnTimeout_ms);
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

	IR_Sniffer_Init();


    SysTick_Config(SystemCoreClock / 1000);	//systick bije co 1ms
    __enable_irq();



    CLOCK_uDelay(2000000);
    PRINTF("/////////////////////\r\nStart petli glownej\r\n/////////////////////\r\n");

    while(1) {

    	int16_t ir_cmd = IR_Process_NonBlocking();
    	// Reakcja tylko, gdy przyszedł autoryzowany i pełny pakiet

//		PRINTF("\r\nOtrzymano komende od pilota: 0x%02X (%u)\r\n", ir_cmd, ir_cmd);
		IR_Chosen_Switch_Action(ir_cmd);


    	//sprawdzanie czy jest ruch
    	if (processDataFlag) {
    		processDataFlag = false;

    		__disable_irq();
    		uint32_t localMax = maxValue;
    		uint32_t localMin = minValue;

    		maxValue = 0;
    		minValue = 0xFFFFFFFF;
			__enable_irq();

			Val = (localMax >= localMin) ? (localMax - localMin) : (localMin - localMax);
//			Val2 = ((Val2 * 1) + Val) / 2;

			//Filtr2 przeciw szybkiemu opadaniu
			if (Val >= finalVal){
				finalVal = Val;
				filtr2DownStep = FILTR2_DOWNSTEP;
				filtr2FallCouter = 0;
			} else{
				if (finalVal >= filtr2DownStep){
					finalVal -= filtr2DownStep;
					filtr2FallCouter++;
					if (filtr2FallCouter > 15) filtr2DownStep += 5;
				} else{
					finalVal = 0;
				}
			}

			// ===================================================
			// 1. DETEKTOR OBWIEDNI (PEAK DETECTOR)
			// ===================================================
//			static uint32_t envelope = 0;
//			if (finalVal > envelope) {
//				envelope = finalVal; // Błyskawiczny wzrost do szczytu fali
//			} else {
//				if (envelope > ENVELOPE_DECAY_RATE) {
//					envelope -= ENVELOPE_DECAY_RATE; // Łagodne opadanie
//				} else {
//					envelope = 0;
//				}
//			}
//
//			// ===================================================
//			// 2. FILTR DOLNOPRZEPUSTOWY OBWIEDNI (WYGŁADZANIE)
//			// ===================================================
//			static uint32_t filteredEnvelope = 0;
//			filteredEnvelope = ((filteredEnvelope * 63) + envelope) >> 6;

			// ===================================================
			// 3. DETEKCJA ZBLIŻANIA / ODDALANIA (TREND)
			// ===================================================
			static uint32_t fastAvg = 0;
			static uint32_t slowAvg = 0;
			int8_t trend = 0; // 1 = Zbliżanie, -1 = Oddalanie, 0 = Stabilnie

			// fastAvg reaguje dynamicznie (okno ~4 próbki), slowAvg stanowi bazę (okno ~16 próbek)
			fastAvg = ((fastAvg * 3) + finalVal) / 4;
			slowAvg = ((slowAvg * 15) + finalVal) / 16;

			if (fastAvg > (slowAvg + TREND_UP_MARGIN)) {
				trend = 1;  // Zbliżanie
			} else if ((fastAvg + TREND_UP_MARGIN) < slowAvg) {
				trend = 2; // Oddalanie
			} else {
				trend = 0;
			}



			Process_Sensor_Data(finalVal, sysSettings.sensitivity, trend);

			if (printDelayCounter == 0) {
				PRINTF("fastAvg = %u | slowAvg = %u | trend = %d | stan = %u\r\n",
							fastAvg, slowAvg, trend, ledState);
				printDelayCounter = UART_PRINT_TIMEOUT;
			}



			// ===================================================
			// MASZYNA STANÓW SYSTEMU
			// ===================================================
			switch (currentMode) {
				case MODE_NORMAL:


					if (ledOffTimeCounter == 0 && sysSettings.standByModeEnable) {
						currentMode = MODE_STANDBY;

//						PRINTF("Brak ruchu przez 30 minut. Przejscie w STANDBY (0%%).\r\n");
					}
					else if (sysSettings.senCalibEnable){
						PRINTF("Kalibracja START!\r\n");
						currentMode = MODE_CALIBRATION;
					}
					break;

				case MODE_STANDBY:
					sysSettings.minDuty = 0;
					sysSettings.maxDuty = 10;

					if (ledState == 1) {
						sysSettings.minDuty = 10;
						sysSettings.maxDuty = 100;
						currentMode = MODE_NORMAL;
					}
					break;

				case MODE_CALIBRATION:
					sysSettings.minDuty = 0;
					sysSettings.maxDuty = 100;
					if (sysSettings.senCalibEnable == false){
						PRINTF("Kalibracja zakonczona!\r\n");
						PRINTF("Ustalone poziomy czulosci:\r\n");
						//poziomy czułości są ustawiane ze stałym odstępem co 5(wartość dobrana empirycznie)
						//wynik kalibracji zapisywany na 2 pozycji listy z poziomami
						for (int i = 0; i < SENSITIVITY_LEVEL_SIZE; i++){
							sysSettings.sensitivityLevel[i] = sysSettings.sensitivity - 5;
							sysSettings.sensitivity += 5;
							PRINTF("%u, ", sysSettings.sensitivityLevel[i]);
						}
						PRINTF("\r\n");
						CLOCK_uDelay(4000000);
						currentMode = MODE_NORMAL;
						sysSettings.minDuty = 10;
						sysSettings.maxDuty = 100;
						sysSettings.sensitivity = sysSettings.sensitivityLevel[1];
					}
					break;



			}
    	}
		__WFI();
	}
    return 0;
}
