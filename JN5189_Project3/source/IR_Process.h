#ifndef IR_PROCESS_H
#define IR_PROCESS_H

#include "fsl_ctimer.h"
#include "fsl_gpio.h"
#include "fsl_iocon.h"

#define MAX_IR_PULSES 150
#define IR_REMOTE_ADDRESS 0x1881
#define IR_NO_DATA -1

void IR_Sniffer_Init(void);
int16_t IR_Process_NonBlocking(void);

#endif
