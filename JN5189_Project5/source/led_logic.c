#include "led_logic.h"

volatile uint32_t noiseFloor = STARTUP_THRESHOLD;
volatile uint32_t ledState = 0;
uint32_t crossCount = 0;

volatile uint16_t ledTimeoutMs = 0;
volatile uint16_t OffDelayTimer = 0;


bool ledTimeoutRstFlag = false;
bool OffToOnDeleyFlag = false;

volatile uint32_t ledOffTimeCounter = 0;




volatile uint8_t currentPwmDuty = 0;  // 10-100%
volatile int8_t pwmDirection = 0;     // 1 = rozjaśnianie, -1 = ściemnianie, 0 = stop

// Struktura konfiguracyjna, której będziemy używać do zmiany wypełnienia
pwm_setup_t pwmChannelSetup;


void PWM_Init_Custom(void) {
    CLOCK_EnableClock(kCLOCK_Iocon);

    // ==========================================
	// Konfiguracja PIO19 jako stałe zasilanie (1)
	// ==========================================
	const uint32_t power_pin_config = (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN);
	IOCON_PinMuxSet(IOCON, 0, 19, power_pin_config);

	gpio_pin_config_t power_config = {kGPIO_DigitalOutput, 1}; // Domyślny stan wysoki (1)
	GPIO_PinInit(GPIO, 0, 19, &power_config);

	// ==========================================
	// Konfiguracja sprzętowego PWM (PIO0 i PIO15)
	// ==========================================
    // Aktywacja IOCON_FUNC4, aby wyprowadzić sygnały PWM0-PU na PIO0 oraz PWM3-PU na PIO15
    const uint32_t pwm_pin_config = (IOCON_FUNC4 | IOCON_MODE_INACT | IOCON_DIGITAL_EN);
    IOCON_PinMuxSet(IOCON, 0, 0, pwm_pin_config);
    IOCON_PinMuxSet(IOCON, 0, 15, pwm_pin_config);

    // Inicjalizacja bazowa modułu PWM
    pwm_config_t pwmConfig;
    PWM_GetDefaultConfig(&pwmConfig);
    PWM_Init(PWM, &pwmConfig);

    // Konfiguracja sposobu działania sprzętowego kanału
    // kPWM_SetLowOnMatchHighOnPeriod -> Pin jest w stanie wysokim na początku cyklu
    // i spada do zera, gdy licznik osiągnie wartość 'comp_val'.
    pwmChannelSetup.pol_ctrl = kPWM_SetHighOnMatchLowOnPeriod;
    pwmChannelSetup.dis_out_level = kPWM_SetHigh;
    pwmChannelSetup.prescaler_val = 31;         // Dzielnik (1 MHz)
    pwmChannelSetup.period_val = PWM_PERIOD;    // Długość cyklu (1000)
    pwmChannelSetup.comp_val = 10;               // Wypełnienie na start 0%

    // Zastosuj konfigurację i uruchom kanały 0 oraz 3
    PWM_SetupPwm(PWM, kPWM_Pwm0, &pwmChannelSetup);
    PWM_SetupPwm(PWM, kPWM_Pwm3, &pwmChannelSetup);

    PWM_StartTimer(PWM, kPWM_Pwm0);
    PWM_StartTimer(PWM, kPWM_Pwm3);
}







void Process_Sensor_Data(uint32_t finalVal, uint32_t sensitivity, int8_t trend) {
    uint32_t dynamicThreshold = noiseFloor + sensitivity;

    if (ledState == 0) {
        noiseFloor = ((noiseFloor * 3) + finalVal) / 4;
        if (finalVal > dynamicThreshold) {
        	crossCount++;
        	if (crossCount >= CROSS_COUNT_MARGINE){
				pwmDirection = 1; // Start rozjaśniania
				ledState = 1;
				ledTimeoutRstFlag = true;
        	}
        }
    } else {
        if (finalVal > dynamicThreshold) {
        	ledTimeoutRstFlag = true;
        }
    }
//    PRINTF("%u\r\n", printDelayCounter);

    printDelayCounter--;
    if (printDelayCounter == 0) {

    	const char* dirStr = (trend == 1) ? "ZBLIZANIE" : ((trend == 2) ? "ODDALANIE" : "STABILNIE");

    	PRINTF("Wartosc = %u |Szum = %u |Prog = %u |Wypelnienie = %u |ColorTemp = %u |minD = %u |maxD = %u |fadeTime = %u |ONtime = %u |StBtime = %u |Czulosc = %u | ",
			   finalVal, noiseFloor, dynamicThreshold, currentPwmDuty, sysSettings.currentColorTemp, sysSettings.minDuty, sysSettings.maxDuty, sysSettings.fadeTime_ms, sysSettings.ledOnTimeout_ms, sysSettings.ledStandByTimeout_ms, sysSettings.sensitivity);

//    	PRINTF("Wypelnienie = %u | Czulosc = %u | minDuty = %u | maxDuty = %u | Stan = %u | Max Czas swiecenia = %u\r\n",
//			   currentPwmDuty, sensitivity, minDuty, maxDuty, ledState, ledOnTimeout_ms);

    }
}

void LED_Process_Timeout(uint32_t ledOnTimeout_ms){
	// ===================================================
	// TIMER 1: Sterowanie czasem świecenia LED
	// ===================================================
	if (ledTimeoutRstFlag){
		ledTimeoutRstFlag = false;
		ledTimeoutMs = ledOnTimeout_ms; //reset licznika
	}
	if (ledTimeoutMs > 0) {
		ledTimeoutMs--;
		if (ledTimeoutMs == 0) {
			pwmDirection = -1; // Start ściemniania
			ledState = 0;
			crossCount = 0;
			OffToOnDeleyFlag = true;	//start opóźnienia
		}
	}
}

//void LED_StayOFF_Timeout(uint16_t OffToOnDelay){
//	// ===================================================
//	// TIMER 3: Delay przed ponownym zapaleniem
//	// ===================================================
//	if (OffToOnDeleyFlag){
//		OffToOnDeleyFlag = false;
//		OffDelayTimer = OffToOnDelay;
//	}
//	if (OffDelayTimer > 0) {
//		OffDelayTimer--;
//	}
//}

void LED_ToStandBy_Timeout(uint32_t ledStandByTimeout_ms){
	// ===================================================
	// TIMER 5: Delay po którym wchodzi w MODE_STANDBY
	// ===================================================
	if (ledState == 0){
		if (ledOffTimeCounter > 0) ledOffTimeCounter--;
	}
	else {
		ledOffTimeCounter = ledStandByTimeout_ms;
	}
}


void LED_Process_Fade(uint8_t minDuty, uint8_t maxDuty, uint16_t fadeTime_ms){
	// ===================================================
	// TIMER 4: Sterowanie czasem rozjaśnienia (zmiany wypełnienia PWM)
	// ===================================================

	uint8_t targetPwmDuty = 0;
	if (ledState == 0) targetPwmDuty = minDuty;
	else if (ledState == 1) targetPwmDuty = maxDuty;


	static uint16_t fadeTimeCounter = 0;
	if (fadeTimeCounter > 0){
		fadeTimeCounter--;
	}
	if (fadeTimeCounter == 0){
		LED_Fade_Action(targetPwmDuty);
		fadeTimeCounter = (fadeTime_ms >= 100) ? (fadeTime_ms / 100) : 1;
	}
}

void LED_Fade_Action(uint8_t targetPwmDuty) {

	if (currentPwmDuty < targetPwmDuty) {
		currentPwmDuty++;
	}
	else if (currentPwmDuty > targetPwmDuty) {
		currentPwmDuty--;
	}
	else return;

	LED_Update_PWM_Hardware();

}

void LED_Update_PWM_Hardware(void) {
    uint32_t min_visible_pwm = (PWM_PERIOD * 10) / 100;
    uint32_t active_range = PWM_PERIOD - min_visible_pwm;
    uint32_t comp_warm = 0;
    uint32_t comp_cold = 0;

    if (currentPwmDuty > 0) {
        uint32_t dutyCold = (currentPwmDuty * sysSettings.currentColorTemp) / 100;
        uint32_t dutyWarm = (currentPwmDuty * (100 - sysSettings.currentColorTemp)) / 100;

        if (dutyWarm > 0) {
            comp_warm = min_visible_pwm + ((dutyWarm * dutyWarm * active_range) / 10000);
        }
        if (dutyCold > 0) {
            comp_cold = min_visible_pwm + ((dutyCold * dutyCold * active_range) / 10000);
        }
    }

    // Rozdzielenie sprzętowe sygnałów PWM
    pwmChannelSetup.comp_val = (uint16_t)comp_warm;
    PWM_SetupPwm(PWM, kPWM_Pwm0, &pwmChannelSetup);

    pwmChannelSetup.comp_val = (uint16_t)comp_cold;
    PWM_SetupPwm(PWM, kPWM_Pwm3, &pwmChannelSetup);
}




