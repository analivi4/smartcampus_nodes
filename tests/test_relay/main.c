#include <stdio.h>
#include "pico/stdlib.h"
#include "relay.h"

#define RELAY_PIN 2

int main(void) {
    stdio_init_all(); sleep_ms(2000);
    printf("=== TESTE CAMADA 3C: Relay ===\n");
    relay_init(RELAY_PIN);

    while(1) {
        relay_set(RELAY_PIN, true);
        printf("Relay LIGADO  — ouça o clique\n");
        sleep_ms(2000);

        relay_set(RELAY_PIN, false);
        printf("Relay DESLIGADO\n");
        sleep_ms(2000);
    }
}