/*
 * ir_raw.c — Transmissor IR por raw timing
 * UFC Quixadá Smart Campus — Nó 1
 */

#include "ir_raw.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

/*
 * Carrier 38kHz: período = 26.3µs → half-period ≈ 13µs
 * Usamos busy-wait com time_us_32() para precisão.
 */
#define CARRIER_HALF_US  13

static void _burst(uint8_t pin, uint32_t duration_us)
{
    uint32_t end = time_us_32() + duration_us;
    while (time_us_32() < end) {
        gpio_put(pin, 1);
        sleep_us(CARRIER_HALF_US);
        gpio_put(pin, 0);
        sleep_us(CARRIER_HALF_US);
    }
}

static void _space(uint8_t pin, uint32_t duration_us)
{
    (void)pin;
    sleep_us(duration_us);
}

void ir_raw_init(uint8_t pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

void ir_raw_send(uint8_t pin, const uint16_t *raw_data, size_t length)
{
    LOG("[IR-RAW] Transmitindo %u pulsos...\n", (unsigned)length);

    for (size_t i = 0; i < length; i++) {
        if (i % 2 == 0)
            _burst(pin, raw_data[i]);
        else
            _space(pin, raw_data[i]);
    }

    gpio_put(pin, 0);
    LOG("[IR-RAW] Frame concluido.\n");
}
