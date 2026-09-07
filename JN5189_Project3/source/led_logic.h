#ifndef LED_LOGIC_H
#define LED_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

#define LED_PORT 0
#define LED_PIN 19
#define STARTUP_THRESHOLD 500
#define PWM_PERIOD 1000



//void LED_Init_Custom(void);
void PWM_Init_Custom(void);
void LED_Fade_Action(uint8_t targetPwmDuty); // Funkcja do SysTicka
void Process_Sensor_Data(uint32_t finalVal, uint32_t sensitivity, int8_t trend);
void LED_Process_Timeout(uint16_t ledOnTimeout);
void LED_StayOFF_Timeout(uint16_t OffToOnDelay);
void LED_Process_Fade(uint8_t minDuty, uint8_t maxDuty, uint16_t fadeTime);

extern volatile uint32_t ledState;


#endif
