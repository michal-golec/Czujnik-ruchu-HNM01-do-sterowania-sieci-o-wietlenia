#ifndef IR_PROCESS_H
#define IR_PROCESS_H

#include "fsl_ctimer.h"
#include "fsl_gpio.h"
#include "fsl_iocon.h"
#include "fsl_debug_console.h"

#define MAX_IR_PULSES 150
#define IR_REMOTE_ADDRESS 0x1881
#define IR_NO_DATA -1
#define IR_REPEAT -2
#define LED_TEMP_CHANGE_STEP 5

extern volatile uint8_t minDuty;
extern volatile uint8_t maxDuty;
extern volatile uint32_t ledOnTimeout;
extern volatile uint32_t ledStandByTimeout;
extern bool standByModeEnable;
extern volatile bool senCalibEnable;

void IR_Sniffer_Init(void);
int16_t IR_Process_NonBlocking(void);
void IR_Chosen_Switch_Action(int16_t ir_cmd);

#endif
