#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "spi_diagnostic.h"



int main(void) {
    stdio_init_all();
    sleep_ms(2000);
    printf("=== TESTE CAMADA 1: Radio SPI ===\n");

    static const spi_diag_config_t cfg = {
        .spi_inst = spi0,
        .sck=18, .mosi=19, .miso=16,
        .nss=17, .reset=15, .dio0=20,
    };
    spi_diagnostic_init(&cfg);
    bool ok = spi_diagnostic_run(&cfg);

    printf("\nRESULTADO: %s\n", ok ? "PASS" : "FAIL");
    while(1) {
        gpio_put(25, ok); sleep_ms(500);
        gpio_put(25, 0);  sleep_ms(500);
    }
}