#include "sen_Calib.h"
#include "led_logic.h"

volatile uint32_t senCalibTime_ms = 0;
volatile uint32_t CalibMaxVal = 0;

void Sensitivity_Calibration_Timer(uint32_t senCalibTimeout_ms){
	if (sysSettings.senCalibEnable){
		sysSettings.sensitivity = 0xFFFFFFF; //znieczulenie na wykrycie ruchu
		if (senCalibTime_ms < senCalibTimeout_ms) {
			senCalibTime_ms++;
			if (finalVal > noiseFloor && finalVal - noiseFloor > CalibMaxVal){
				CalibMaxVal = finalVal - noiseFloor;
			}

		}
		else {
			senCalibTime_ms = 0;
			sysSettings.senCalibEnable = false;
			sysSettings.sensitivity = CalibMaxVal + CALIBR_MARGINE;
			CalibMaxVal = 0;
		}
	}
	else return;
}
