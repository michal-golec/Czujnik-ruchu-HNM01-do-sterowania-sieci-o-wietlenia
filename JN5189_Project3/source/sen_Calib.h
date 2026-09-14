#ifndef SEN_CALIB_H
#define SEN_CALIB_H

#include <stdint.h>
#include <stdbool.h>

#define SENSITIVITY_MARGIN 80

extern uint32_t sensitivity;

void Sensitivity_Calibration_Timer(bool senCalibEnable,
		uint32_t senCalibTimeStep, uint32_t senCalibStep);

#endif
