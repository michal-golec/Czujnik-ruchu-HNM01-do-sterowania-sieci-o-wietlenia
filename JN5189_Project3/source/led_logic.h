#ifndef LED_LOGIC_H
#define LED_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

#define LED_PORT 0
#define LED_PIN 19
#define SENSITIVITY_MARGIN 80
#define STARTUP_THRESHOLD 500
#define PWM_PERIOD 1000
#define FADE_DIVIDER 30 // rozjaśnienie przez FADE_DIVIDER * 100ms


//void LED_Init_Custom(void);
void PWM_Init_Custom(void);
void LED_Fade_Action(uint8_t targetPwmDuty); // Funkcja do SysTicka
void Process_Sensor_Data(uint32_t finalVal);
void LED_Process_Timeout(uint16_t ledOnTimeout);
void LED_StayOFF_Timeout(uint16_t OffToOnDelay);
void LED_Process_Fade(uint8_t targetPwmDuty, uint16_t fadeTime);


#endif
