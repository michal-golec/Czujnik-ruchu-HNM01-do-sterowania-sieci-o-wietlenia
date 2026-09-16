#include "sen_Calib.h"
#include "led_logic.h"

volatile uint32_t senCalibTime_ms = 0;
volatile uint32_t CalibMaxVal = 0;

void Sensitivity_Calibration_Timer(volatile bool *senCalibEnable_ptr, uint32_t senCalibTimeout_ms){
	if (*senCalibEnable_ptr){
		sensitivity = 0xFFFFFFF; //znieczulenie na wykrycie ruchu
		if (senCalibTime_ms < senCalibTimeout_ms) {
			senCalibTime_ms++;
			if (finalVal > noiseFloor && finalVal - noiseFloor > CalibMaxVal){
				CalibMaxVal = finalVal - noiseFloor;
			}

		}
		else {
			senCalibTime_ms = 0;
			*senCalibEnable_ptr = false;
			sensitivity = CalibMaxVal + CALIBR_MARGINE;
			CalibMaxVal = 0;
		}
	}
	else return;
}
