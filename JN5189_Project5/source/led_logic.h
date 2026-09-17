#ifndef LED_LOGIC_H
#define LED_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "fsl_iocon.h"
#include "fsl_debug_console.h"
#include "fsl_pwm.h"
#include "fsl_gpio.h"

#define LED_PORT 0
#define LED_PIN 19
#define STARTUP_THRESHOLD 500
#define PWM_PERIOD 1000
#define CROSS_COUNT_MARGINE 2
#define SENSITIVITY_LEVEL_SIZE 5

typedef struct {
	volatile uint32_t ledOnTimeout_ms;
	volatile uint32_t ledStandByTimeout_ms;
    volatile uint8_t currentColorTemp;
    volatile uint8_t minDuty;
    volatile uint8_t maxDuty;
    volatile uint16_t fadeTime_ms;
    volatile bool senCalibEnable;
    bool standByModeEnable;
    uint32_t sensitivity;
    uint32_t sensitivityLevel[SENSITIVITY_LEVEL_SIZE];
} SystemSettings_t;

extern SystemSettings_t sysSettings;


extern volatile uint32_t ledOffTimeCounter;
extern uint32_t printDelayCounter;
extern volatile uint32_t ledState;
extern volatile uint32_t noiseFloor;


void PWM_Init_Custom(void);
void LED_Fade_Action(uint8_t targetPwmDuty); // Funkcja do SysTicka
void Process_Sensor_Data(uint32_t finalVal, uint32_t sensitivity, int8_t trend);
void LED_Process_Timeout(uint32_t ledOnTimeout_ms);
void LED_ToStandBy_Timeout(uint32_t ledStandByTimeout_ms);
void LED_Process_Fade(uint8_t minDuty, uint8_t maxDuty, uint16_t fadeTime_ms);
void LED_Update_PWM_Hardware(void);


#endif
