#include "led_logic.h"
#include "fsl_iocon.h"
#include "fsl_debug_console.h"
#include "fsl_pwm.h"
#include "fsl_gpio.h"

uint32_t noiseFloor = STARTUP_THRESHOLD - SENSITIVITY_MARGIN;
volatile uint32_t state = 0;
volatile uint16_t ledTimeoutMs = 0;
volatile uint16_t OffDelayTimer = 0;
uint32_t printDelayCounter = 0;
bool ledTimeoutRstFlag = false;
bool OffToOnDeleyFlag = false;


volatile uint8_t currentPwmDuty = 10;  // 10-100%
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



void LED_Process_Fade(uint8_t targetPwmDuty) {

	if (currentPwmDuty < targetPwmDuty) {
		currentPwmDuty++;
	}
	else if (currentPwmDuty > targetPwmDuty) {
		currentPwmDuty--;
	}
	else return;

	// Ponieważ JN5189 nie ma osobnej funkcji PWM_UpdatePwmDutycycle,
	// wyliczamy nową wartość odcięcia (0 - 1000) i aplikujemy ją do modułu:
	pwmChannelSetup.comp_val = (currentPwmDuty * PWM_PERIOD) / 100;
	PWM_SetupPwm(PWM, kPWM_Pwm0, &pwmChannelSetup);
	PWM_SetupPwm(PWM, kPWM_Pwm3, &pwmChannelSetup);
}



void Process_Sensor_Data(uint32_t finalVal) {
    uint32_t dynamicThreshold = noiseFloor + SENSITIVITY_MARGIN;

    if (state == 0) {
        noiseFloor = ((noiseFloor * 31) + finalVal) / 32;
        if (finalVal > dynamicThreshold && OffDelayTimer == 0) {
        	pwmDirection = 1; // Start rozjaśniania
            state = 1;
            ledTimeoutRstFlag = true;
        }
    } else {
        if (finalVal > dynamicThreshold) {
        	ledTimeoutRstFlag = true;
        }
    }

    printDelayCounter++;
    if (printDelayCounter >= 0) {
        PRINTF("Szum: %u | Roznica: %u | Prog: %u | Stan: %u | Wypelnienie: %u\r\n",
        		noiseFloor, finalVal, dynamicThreshold, state, currentPwmDuty);
        printDelayCounter = 0;
    }
}

void LED_Process_Timeout(uint16_t ledOnTimeout){
	// ===================================================
	// TIMER 1: Sterowanie czasem świecenia LED
	// ===================================================
	if (ledTimeoutRstFlag){
		ledTimeoutRstFlag = false;
		ledTimeoutMs = ledOnTimeout; //reset licznika
	}
	if (ledTimeoutMs > 0) {
		ledTimeoutMs--;
		if (ledTimeoutMs == 0) {
			pwmDirection = -1; // Start ściemniania
			state = 0;
			OffToOnDeleyFlag = true;	//start opóźnienia
		}
	}
}

void LED_StayOFF_Timeout(uint16_t OffToOnDelay){
	// ===================================================
	// TIMER 3: Delay przed ponownym zapaleniem
	// ===================================================
	if (OffToOnDeleyFlag){
		OffToOnDeleyFlag = false;
		OffDelayTimer = OffToOnDelay;
	}
	if (OffDelayTimer > 0) {
		OffDelayTimer--;
	}
}


void LED_Fade_Timeout(uint8_t targetPwmDuty, uint16_t fadeTime){
	// ===================================================
	// TIMER 4: Sterowanie czasem rozjaśnienia (zmiany wypełnienia PWM)
	// ===================================================
	uint16_t fadeTimeoutDivider = fadeTime;
	if (fadeTimeoutDivider > 0){
		fadeTimeoutDivider--;
		if (fadeTimeoutDivider == 0){
			LED_Process_Fade(targetPwmDuty);
			fadeTimeoutDivider = fadeTime;
		}
	}
}
