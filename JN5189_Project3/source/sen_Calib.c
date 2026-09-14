#include "sen_Calib.h"

volatile uint32_t sensitivityCalibTimeout = 5000;

void Sensitivity_Calibration_Timer(bool senCalibEnable, uint32_t senCalibTimeStep, uint32_t senCalibStep){
	if (senCalibEnable){
		if (sensitivityCalibTimeout > 0) sensitivityCalibTimeout--;
		else {
			sensitivityCalibTimeout = senCalibTimeStep;
			sensitivity = sensitivity - senCalibStep;
			if (sensitivity == 0) sensitivity = SENSITIVITY_MARGIN;
		}
	}
	else return;
}
