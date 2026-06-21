#include <stdio.h>
#include "pico/stdlib.h"
#include "dht11.h"
#include "hardware/sync.h"


#define DHT11_PIN 22

int main(void) {
    stdio_init_all(); sleep_ms(2000);
    printf("=== TESTE CAMADA 3A: DHT11 ===\n");
    dht11_init(DHT11_PIN);

    while(1) {
        dht11_result_t r;
        
        dht11_status_t s = dht11_read(DHT11_PIN, &r);
        

        if (s == DHT11_OK)
            printf("PASS — Temp: %.1f°C  Umidade: %.1f%%\n", r.temperature, r.humidity);
        else
            printf("FAIL — %s\n", dht11_status_str(s));

        sleep_ms(2000); /* DHT11 precisa de >=2s entre leituras */
    }
}