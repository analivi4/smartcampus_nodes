/*
 * lorawan_task.c — Nó 3: Contagem de Pessoas VL53L0X (somente uplink)
 * UFC Quixadá Smart Campus
 */

#include "lorawan_task.h"
#include "pico/lorawan.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/stdlib.h"
#include <stdio.h>

static const struct lorawan_sx1276_settings sx1276_settings = {
    .spi   = { .inst = spi0, .mosi = 19, .miso = 16, .sck = 18, .nss = 17 },
    .reset = 15, .dio0 = 20, .dio1 = 21,
};

static const struct lorawan_abp_settings abp_settings = {
    .device_address      = "01216544",
    .network_session_key = "4C45CC20EF7B371C27C2CADBC9CFE486",
    .app_session_key     = "787093B500D63CCD81FE88D338BED172",
    .channel_mask        = "FF000000000000000000",   /* AU915 FSB2 */
};

extern SemaphoreHandle_t xLoRaInitSemaphore;
static volatile bool is_joined = false;

void LoRaWan_Send(uint8_t *data, uint8_t size)
{
    if (!data || size == 0 || !is_joined) return;
    int r = lorawan_send_unconfirmed(data, size, 1);
    LOG("[No3-LoRa] %s (%d bytes)\n", r == 0 ? "Uplink OK" : "Erro uplink", size);
}

void vLoRaWanTask(void *pvParameters)
{
    LOG("[No3-LoRa] Inicializando (ABP, AU915)...\n");

    lorawan_debug(true);

    if (lorawan_init_abp(&sx1276_settings, LORAMAC_REGION_AU915, &abp_settings) != 0) {
        LOG("[No3-LoRa] ERRO FATAL: init ABP.\n");
        vTaskDelete(NULL); return;
    }

    lorawan_join();
    for (uint32_t t = 0; !lorawan_is_joined() && t < 100; t++) {
        lorawan_process();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (!lorawan_is_joined()) {
        LOG("[No3-LoRa] ERRO: Join nao confirmado.\n");
        vTaskDelete(NULL); return;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    LOG("[No3-LoRa] Join OK.\n");
    is_joined = true;
    if (xLoRaInitSemaphore) xSemaphoreGive(xLoRaInitSemaphore);

    while (1) {
        lorawan_process();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
