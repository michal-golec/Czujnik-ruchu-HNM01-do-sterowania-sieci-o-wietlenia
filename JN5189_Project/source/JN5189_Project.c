#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_adc.h"

//Def global
#define BAZOWY_ADC ADC0
#define KANAL_ADC 2
#define PROG_CZULOSCI 150

int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();


    //zmiana portu
    //config ADC
    CLOCK_EnableClock(kCLOCK_Adc0);
    POWER_EnablePD(kPDRUNCFG_PD_LDO_ADC_EN);
	PMC->PDRUNCFG |= PMC_PDRUNCFG_ENA_LDO_ADC_MASK;

	// 2. (Opcjonalnie, ale mocno zalecane) Przeprowadź kalibrację
	// Uwaga: W zależności od MCU może to wymagać podania częstotliwości jako drugiego argumentu

//#if !(defined(FSL_FEATURE_ADC_HAS_NO_CALIB_FUNC) && FSL_FEATURE_ADC_HAS_NO_CALIB_FUNC)
//#if defined(FSL_FEATURE_ADC_HAS_CALIB_REG) && FSL_FEATURE_ADC_HAS_CALIB_REG
//    /* Calibration after power up. */
//    if (ADC_DoSelfCalibration(DEMO_ADC_BASE))
//#else
//    uint32_t frequency;
//#if defined(SYSCON_ADCCLKDIV_DIV_MASK)
//    frequency = CLOCK_GetFreq(DEMO_ADC_CLOCK_SOURCE) / CLOCK_GetClkDivider(kCLOCK_DivAdcClk);
//#else
//    frequency = CLOCK_GetFreq(DEMO_ADC_CLOCK_SOURCE);
//#endif /* SYSCON_ADCCLKDIV_DIV_MASK */
//    /* Calibration after power up. */
//    if (ADC_DoSelfCalibration(DEMO_ADC_BASE, frequency))
//#endif /* FSL_FEATURE_ADC_HAS_CALIB_REG */
//    {
//        PRINTF("ADC_DoSelfCalibration() Done.\r\n");
//    }
//    else
//    {
//        PRINTF("ADC_DoSelfCalibration() Failed.\r\n");
//    }
//#endif /* FSL_FEATURE_ADC_HAS_NO_CALIB_FUNC */


	adc_config_t configuration;
	ADC_GetDefaultConfig(&configuration);
	configuration.clockDividerNumber = 7;
	ADC_Init(BAZOWY_ADC, &configuration);

	// 4. Skonfiguruj sekwencję - ZWRÓĆ UWAGĘ NA "= {0}" (zeruje całą strukturę!)
	adc_conv_seq_config_t adcConvSeqAConfigStruct = {0};
	adcConvSeqAConfigStruct.channelMask = (1U << KANAL_ADC);
	adcConvSeqAConfigStruct.triggerMask = 0U; // 0 -- wyzwalanie ręczne
	adcConvSeqAConfigStruct.triggerPolarity = kADC_TriggerPolarityPositiveEdge;
	adcConvSeqAConfigStruct.interruptMode = kADC_InterruptForEachSequence;
	adcConvSeqAConfigStruct.enableSingleStep = false;
	adcConvSeqAConfigStruct.enableSyncBypass = false;
        ///////////////////////////////////

	//save & Seq Start
	ADC_SetConvSeqAConfig(BAZOWY_ADC, &adcConvSeqAConfigStruct);
	ADC_EnableConvSeqA(BAZOWY_ADC, true);



    //def
//    uint32_t MW_SENSOR;
    adc_result_info_t adcResultInfoStruct;


    while(1) {

    	//narazie sekwencja softwarowa
    	//TODO: wyzwalanie normalnie z timera
    	ADC_DoSoftwareTriggerConvSeqA(BAZOWY_ADC);

    	// W while() zeby nie bylo opoznien pomiaru
    	while (!ADC_GetChannelConversionResult(BAZOWY_ADC, KANAL_ADC, &adcResultInfoStruct)) {
    	        }

    	PRINTF("Wartosc ADC: %d\r\n", adcResultInfoStruct.result);

//    	MW_SENSOR = GPIO_PinRead(GPIO, 0, 16);
//    	PRINTF("%d\r\n", MW_SENSOR);

    }
    return 0;
}
