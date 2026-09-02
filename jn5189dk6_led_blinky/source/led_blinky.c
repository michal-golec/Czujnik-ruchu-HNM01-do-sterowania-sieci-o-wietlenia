/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"
#include "fsl_gpio.h"

#include "pin_mux.h"
#include <stdbool.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_USART USART0
#define DEMO_USART_CLK_SRC kCLOCK_Fro32M

/* Can't use GREEN LED as conflicts with SW2 usage */
#define BOARD_LED_PORT 0
#define BOARD_LED_PIN 19


volatile uint32_t g_systickCounter;


void SysTick_Handler(void)
{
    if (g_systickCounter != 0U)
    {
        g_systickCounter--;
    }
}

void SysTick_DelayTicks(uint32_t n)
{
    g_systickCounter = n;
    while (g_systickCounter != 0U)
    {
    }
}


int main(void)
{
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput,
        0,
    };

    /* Board pin init */
    /* Security code to allow debug access */
    SYSCON->CODESECURITYPROT = 0x87654320;


    /* reset FLEXCOMM for USART */
//    RESET_PeripheralReset(kFC0_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kGPIO0_RST_SHIFT_RSTn);

    BOARD_BootClockRUN();
//    BOARD_InitDebugConsole();
    BOARD_InitPins();

    /* Init output LED GPIO. */
    GPIO_PortInit(GPIO, BOARD_LED_PORT);
    GPIO_PinInit(GPIO, BOARD_LED_PORT, BOARD_LED_PIN, &led_config);

    /* Set systick reload value to generate 1ms interrupt */
    if (SysTick_Config(SystemCoreClock / 1000U))
    {
        while (1)
        {
        }
    }

    while (1)
    {
        /* Delay 1000 ms */
        SysTick_DelayTicks(1000U);
//        GPIO_PortW(GPIO, BOARD_LED_PORT, 1u << BOARD_LED_PIN);
        GPIO_PinWrite(GPIO, BOARD_LED_PORT, BOARD_LED_PIN, 1);
        SysTick_DelayTicks(1000U);
        GPIO_PinWrite(GPIO, BOARD_LED_PORT, BOARD_LED_PIN, 0);
    }
}
