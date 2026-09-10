#include "IR_Process.h"

//zmienne do pilota
uint32_t pulse_durations[MAX_IR_PULSES];
volatile uint16_t pulse_index = 0;


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

// Zmienne globalne modułu IR (zakładam, że je masz)
// uint32_t pulse_durations[MAX_IR_PULSES];
// volatile uint16_t pulse_index = 0;

int16_t IR_Process_NonBlocking(void) {
    static uint8_t last_pin_state = 1;
    static uint32_t last_edge_time = 0;

    // Domyślny stan zwrotny - brak nowych danych
    int16_t received_command = IR_NO_DATA;

    uint8_t current_state = GPIO_PinRead(GPIO, 0, 14);
    uint32_t current_time = CTIMER_GetTimerCountValue(CTIMER0);

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
            // Zabezpieczamy się przed ramkami powtórzeń i śmieciami (< 66 impulsów)
            if (pulse_index >= 66) {
                uint16_t address = 0;
                uint8_t command = 0;
                uint8_t inv_command = 0;

                // A. Parsowanie adresu pilota (pierwsze 16 bitów / indeksy 4 do 34)
                for (uint16_t i = 0; i < 16; i++) {
                    address <<= 1;
                    if (pulse_durations[4 + (i * 2)] > 1500) address |= 1;
                }

                // B. Parsowanie komendy (kolejne 8 bitów / indeksy 36 do 50)
                for (uint16_t i = 0; i < 8; i++) {
                    command <<= 1;
                    if (pulse_durations[36 + (i * 2)] > 1500) command |= 1;
                }

                // C. Parsowanie negacji komendy (ostatnie 8 bitów / indeksy 52 do 66)
                for (uint16_t i = 0; i < 8; i++) {
                    inv_command <<= 1;
                    if (pulse_durations[52 + (i * 2)] > 1500) inv_command |= 1;
                }

                // ===================================================
                // 3. WALIDACJA DANYCH
                // ===================================================
                // Komenda musi być logicznym przeciwieństwem inv_command.
                // Używamy sumy bitowej, aby zniwelować zakłócenia sprzętowe.
                if ((address == IR_REMOTE_ADDRESS) && (command == (uint8_t)(~inv_command))) {
                    received_command = command; // Ramka prawidłowa!
                }
            }

            // Reset bufora przygotowujący na kolejne pakiety
            pulse_index = 0;
        }
    }

    return received_command;
}
