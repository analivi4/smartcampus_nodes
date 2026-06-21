/*
 * ir_nec.c — Transmissor IR protocolo NEC
 * UFC Quixadá Smart Campus — Nó 1
 */

#include "ir_nec.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

/* Período do carrier 38kHz ≈ 26µs (13µs HIGH + 13µs LOW) */
#define CARRIER_HALF_US  13

/* Duração dos pulsos e espaços NEC */
#define LEADER_BURST_US  9000
#define LEADER_SPACE_US  4500
#define BIT_BURST_US      562
#define BIT_ONE_SPACE_US 1687
#define BIT_ZERO_SPACE_US 562
#define STOP_BURST_US     562

/* Emite burst de carrier 38kHz por 'duration_us' microssegundos */
static void _burst(uint8_t pin, uint32_t duration_us)
{
    uint32_t end = time_us_32() + duration_us;
    while (time_us_32() < end) {
        gpio_put(pin, 1); sleep_us(CARRIER_HALF_US);
        gpio_put(pin, 0); sleep_us(CARRIER_HALF_US);
    }
}

/* Espaço (sem sinal) por 'duration_us' microssegundos */
static void _space(uint8_t pin, uint32_t duration_us)
{
    gpio_put(pin, 0);
    sleep_us(duration_us);
}

/* Envia 1 byte LSB first */
static void _send_byte(uint8_t pin, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        _burst(pin, BIT_BURST_US);
        if (byte & (1 << i)) {
            _space(pin, BIT_ONE_SPACE_US);
        } else {
            _space(pin, BIT_ZERO_SPACE_US);
        }
    }
}

void ir_nec_init(uint8_t pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

void ir_nec_send(uint8_t pin, uint8_t address, uint8_t command)
{
    printf("[IR-NEC] Enviando addr=0x%02X cmd=0x%02X\n", address, command);

    /* Leader code */
    _burst(pin, LEADER_BURST_US);
    _space(pin, LEADER_SPACE_US);

    /* Dados: address, ~address, command, ~command */
    _send_byte(pin, address);
    _send_byte(pin, ~address);
    _send_byte(pin, command);
    _send_byte(pin, ~command);

    /* Stop bit */
    _burst(pin, STOP_BURST_US);
    _space(pin, STOP_BURST_US);

    printf("[IR-NEC] Frame completo.\n");
}
