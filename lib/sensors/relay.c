/*
 * relay.c — Controle de relay via GPIO
 * UFC Quixadá Smart Campus — Nó 2
 */

#include "relay.h"
#include "hardware/gpio.h"
#include <stdio.h>

void relay_init(uint8_t pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
    LOG("[Relay] Inicializado no GPIO%d.\n", pin);
}

void relay_set(uint8_t pin, bool on)
{
    gpio_put(pin, on ? 1 : 0);
    LOG("[Relay] GPIO%d -> %s\n", pin, on ? "LIGADO" : "DESLIGADO");
}

bool relay_get(uint8_t pin)
{
    return gpio_get(pin) != 0;
}

void relay_toggle(uint8_t pin)
{
    bool current = relay_get(pin);
    relay_set(pin, !current);
}
