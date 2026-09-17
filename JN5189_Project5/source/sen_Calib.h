#ifndef SEN_CALIB_H
#define SEN_CALIB_H

#include <stdint.h>
#include <stdbool.h>

#define CALIBR_MARGINE 10

extern uint32_t finalVal;
extern volatile uint32_t CalibMaxVal;
extern volatile uint32_t senCalibTime_ms;

void Sensitivity_Calibration_Timer(uint32_t senCalibTimeout_ms);

#endif
