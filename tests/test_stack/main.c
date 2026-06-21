#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/lorawan.h"
#include "FreeRTOS.h"
#include "task.h"

uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((aligned(8)));

static const struct lorawan_sx1276_settings sx1276 = {
    .spi = {.inst=spi0, .mosi=19, .miso=16, .sck=18, .nss=17},
    .reset=15, .dio0=20, .dio1=21,
};
static const struct lorawan_abp_settings abp = {
    .device_address      = "AABBCC01",
    .network_session_key = "00000000000000000000000000000001",
    .app_session_key     = "00000000000000000000000000000001",
    .channel_mask        = "FF000000000000000000",
};

static void vTestTask(void *p) {
    printf("[Stack] Inicializando ABP...\n");
    lorawan_debug(true);
    
    lorawan_erase_nvm();
    sleep_ms(100);

    if (lorawan_init_abp(&sx1276, LORAMAC_REGION_AU915, &abp) != 0) {
        printf("FAIL — lorawan_init_abp\n"); vTaskDelete(NULL); return;
    }
    lorawan_join();
    for (uint32_t t = 0; !lorawan_is_joined() && t < 100; t++) {
        lorawan_process(); vTaskDelay(pdMS_TO_TICKS(100));
    }
    printf(lorawan_is_joined() ? "PASS — Join ABP OK\n" : "FAIL — Join nao confirmado\n");

    /* Tenta enviar um pacote — vai falhar no ar sem gateway, mas não deve travar */
    uint8_t payload[] = "TESTE";
    int r = lorawan_send_unconfirmed(payload, sizeof(payload), 1);
    printf("lorawan_send_unconfirmed retornou: %d (0=enfileirado)\n", r);

    while(1) { lorawan_process(); vTaskDelay(pdMS_TO_TICKS(10)); }
}

int main(void) {
    stdio_init_all(); sleep_ms(2000);
    printf("=== TESTE CAMADA 2: LoRaWAN Stack ===\n");
    xTaskCreate(vTestTask, "Test", 4096, NULL, 4, NULL);
    vTaskStartScheduler();
    while(1);
}