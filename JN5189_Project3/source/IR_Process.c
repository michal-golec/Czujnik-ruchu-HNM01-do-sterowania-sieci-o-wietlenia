#include "IR_Process.h"

//zmienne do pilota
uint32_t pulse_durations[MAX_IR_PULSES];
volatile uint16_t pulse_index = 0;
uint8_t current_state = 0;
uint32_t current_time = 0;


void IR_Sniffer_Init(void) {
    CLOCK_EnableClock(kCLOCK_Iocon);

    // Konfiguracja PIO14: GPIO, pull-up, włączony bufor cyfrowy
    const uint32_t ir_pin_config = (IOCON_FUNC0 | IOCON_MODE_PULLUP | IOCON_DIGITAL_EN);
    IOCON_PinMuxSet(IOCON, 0, 14, ir_pin_config);

    gpio_pin_config_t ir_config = {kGPIO_DigitalInput, 0};
    GPIO_PinInit(GPIO, 0, 14, &ir_config);

    // Konfiguracja CTIMER0 jako wolnobieżny licznik
    ctimer_config_t ctimer_config;
    CTIMER_GetDefaultConfig(&ctimer_config);
    ctimer_config.prescale = 31; // Dzielnik zegara (32 MHz / 32 = 1 MHz -> 1 us / tick)
    CTIMER_Init(CTIMER0, &ctimer_config);
    CTIMER_StartTimer(CTIMER0);
}

int16_t IR_Process_NonBlocking(void) {
    static uint8_t last_pin_state = 1;
    static uint32_t last_edge_time = 0;
    int16_t received_command = IR_NO_DATA;

    current_state = GPIO_PinRead(GPIO, 0, 14);
    current_time = CTIMER_GetTimerCountValue(CTIMER0);



    // ===================================================
    // 1. ZBIERANIE CZASÓW ZBOCZY (NON-BLOCKING)
    // ===================================================
    if (current_state != last_pin_state) {
        uint32_t duration = (current_time >= last_edge_time) ?
                            (current_time - last_edge_time) :
                            (0xFFFFFFFF - last_edge_time + current_time + 1);

        last_edge_time = current_time;

        if (pulse_index < MAX_IR_PULSES) {
            pulse_durations[pulse_index++] = duration;
        }
        last_pin_state = current_state;
    }

    // ===================================================
    // 2. WYKRYCIE KOŃCA RAMKI I DEKODOWANIE
    // ===================================================
    if ((current_state == 1) && (pulse_index > 0)) {
        uint32_t idle_time = (current_time >= last_edge_time) ?
                             (current_time - last_edge_time) :
                             (0xFFFFFFFF - last_edge_time + current_time + 1);

        // Jeśli minęło 20ms ciszy, analizujemy zebrany bufor
        if (idle_time > 20000) {

            // Protokół NEC z adresem 16-bitowym wymaga 32 bitów danych.
            // 32 bity * 2 krawędzie = 64 impulsy + 4 na nagłówek = minimum 68 impulsów.
            // Zabezpieczenie przed ramkami powtórzeń i śmieciami (< 66 impulsów)
            if (pulse_index >= 66) {
                uint16_t address = 0;
                uint8_t command = 0;
                uint8_t inv_command = 0;

                // Odczyt adresu pilota (pierwsze 16 bitów / indeksy 4 do 34)
                for (uint16_t i = 0; i < 16; i++) {
                    address <<= 1;
                    if (pulse_durations[4 + (i * 2)] > 1500) address |= 1;
                }

                // Odczyt komendy (kolejne 8 bitów / indeksy 36 do 50)
                for (uint16_t i = 0; i < 8; i++) {
                    command <<= 1;
                    if (pulse_durations[36 + (i * 2)] > 1500) command |= 1;
                }

                // Odczyt negacji komendy (ostatnie 8 bitów / indeksy 52 do 66)
                for (uint16_t i = 0; i < 8; i++) {
                    inv_command <<= 1;
                    if (pulse_durations[52 + (i * 2)] > 1500) inv_command |= 1;
                }

                // ===================================================
                // 3. WALIDACJA DANYCH
                // ===================================================
                if ((address == IR_REMOTE_ADDRESS) && (command == (uint8_t)(~inv_command))) {
                    received_command = command; // Ramka prawidłowa!
                }
            }

            else if (pulse_index >2 && pulse_index < 6) {
            	received_command = IR_REPEAT;
            }

            pulse_index = 0;
        }
    }

    return received_command;
}




void IR_Chosen_Switch_Action(int16_t ir_cmd){
	static int16_t last_valid_command = IR_NO_DATA;
	static bool waiting_for_repeat = false;
	static uint32_t wait_start_time = 0;

	if (ir_cmd != IR_REPEAT && ir_cmd != IR_NO_DATA){
		last_valid_command = ir_cmd;

		if (ir_cmd == 0xB8){
			wait_start_time = CTIMER_GetTimerCountValue(CTIMER0);
			waiting_for_repeat = true;
		}
		else {
			switch (last_valid_command){
				case 0x8:
					PRINTF("ON\r\n");
					break;
				case 0x80:
					PRINTF("Auto\r\n");
					break;
				case 0x60:
					PRINTF("Reset\r\n");
					break;
				case 0x24:
					PRINTF("Power%% up\r\n");
					break;
				case 0x44:
					PRINTF("Power%% down\r\n");
					break;
				case 0x94:
					PRINTF("Memory\r\n");
					break;
				case 0x90:
					PRINTF("Detection Range 100%%\r\n");
					break;
				case 0xF8:
					PRINTF("Detection Range 50%%\r\n");
					break;
				case 0xB0:
					PRINTF("Detection Range 25%%\r\n");
					break;
				case 0x68:
					PRINTF("Daylight sensor 1000lux\r\n");
					break;
				case 0x48:
					PRINTF("Daylight sensor 500lux\r\n");
					break;
				case 0xE8:
					PRINTF("Daylight sensor 400lux\r\n");
					break;
				case 0xA8:
					PRINTF("Daylight sensor 300lux\r\n");
					break;
				case 0x88:
					PRINTF("Daylight sensor 200lux\r\n");
					break;
				case 0xD8:
					PRINTF("Daylight sensor 150lux\r\n");
					break;
				case 0x98:
					PRINTF("Daylight sensor 100lux\r\n");
					break;
				case 0xB2:
					PRINTF("Daylight sensor DISABLE\r\n");
					break;
				case 0x2:
					PRINTF("Hold time TEST 3s\r\n");
					break;
				case 0x32:
					PRINTF("Hold time 30s\r\n");
					break;
				case 0x50:
					PRINTF("Hold time 90s\r\n");
					break;
				case 0x78:
					PRINTF("Hold time 5min\r\n");
					break;
				case 0x38:
					PRINTF("Hold time 10min\r\n");
					break;
				case 0x28:
					PRINTF("Hold time 30min\r\n");
					break;
				case 0x20:
					PRINTF("Dim off 10s\r\n");
					break;
				case 0x4:
					PRINTF("Dim off 5min\r\n");
					break;
				case 0x70:
					PRINTF("Dim off 10min\r\n");
					break;
				case 0x58:
					PRINTF("Dim off 30min\r\n");
					break;
				case 0xF0:
					PRINTF("Dim off 1h\r\n");
					break;
				case 0x30:
					PRINTF("Dim off +INFINITY\r\n");
					break;
				case 0x40:
					PRINTF("Dim level 0%%\r\n");
					break;
				case 0x12:
					PRINTF("Dim level 10%%\r\n");
					break;
				case 0x2A:
					PRINTF("Dim level 30%%\r\n");
					break;
				case 0xA0:
					PRINTF("Dim level 50%%\r\n");
					break;
			}
		}
	}

	else if (ir_cmd == IR_REPEAT && last_valid_command != IR_NO_DATA) {
		if (waiting_for_repeat) {
			waiting_for_repeat = false;
			PRINTF("Detection Range 75%%\r\n");
			// Tutaj logika dla 75%
		} else {
			// Zwykłe przytrzymanie innego przycisku
//			PRINTF("Powtorzenie: 0x%02X\r\n", last_valid_command);
			switch (last_valid_command){
				case 0x24:
					PRINTF("Power%% up\r\n");
					break;
				case 0x44:
					PRINTF("Power%% down\r\n");
					break;
			}
		}
	}

	if (waiting_for_repeat) {
		current_time = CTIMER_GetTimerCountValue(CTIMER0);
		uint32_t elapsed = (current_time >= wait_start_time) ?
						   (current_time - wait_start_time) :
						   (0xFFFFFFFF - wait_start_time + current_time + 1);
		// Jeśli minęło 150 000 us (150 ms) i nie było powtórzenia - to jest Apply
		if (elapsed > 150000) {
			waiting_for_repeat = false;
			PRINTF("Apply\r\n");
			// Tutaj logika dla Apply
		}
	}





}






