/*
 * lorawan_task.c — Nó 2: Estado do Relay (uplink) + Controle Remoto (downlink)
 * UFC Quixadá Smart Campus
 *
 * Padrão: Diogoassun/Pico_LoRaWAN
 */

#include "lorawan_task.h"
#include "pico/lorawan.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

static const struct lorawan_sx1276_settings sx1276_settings = {
    .spi   = { .inst = spi0, .mosi = 19, .miso = 16, .sck = 18, .nss = 17 },
    .reset = 15, .dio0 = 20, .dio1 = 21,
};

static const struct lorawan_abp_settings abp_settings = {
    .device_address      = "01216545",
    .network_session_key = "4C45CC20EF7B371C27C2CADBC9CFE486",
    .app_session_key     = "787093B500D63CCD81FE88D338BED172",
    .channel_mask        = "FF000000000000000000",   /* AU915 FSB2 */
};

extern SemaphoreHandle_t xLoRaInitSemaphore;

static volatile bool     is_joined          = false;
static uint8_t           downlink_buf[64]   = {0};
static uint8_t           downlink_len       = 0;
static volatile bool     downlink_available = false;
static SemaphoreHandle_t xDownlinkMutex     = NULL;

void LoRaWan_Send(uint8_t *data, uint8_t size)
{
    if (!data || size == 0 || !is_joined) return;
    int r = lorawan_send_unconfirmed(data, size, 1);
    LOG("[No2-LoRa] %s (%d bytes)\n", r == 0 ? "Uplink OK" : "Erro uplink", size);
}

bool LoRaWan_GetDownlink(uint8_t *buf, uint8_t *len)
{
    if (!xDownlinkMutex) return false;
    bool got = false;
    if (xSemaphoreTake(xDownlinkMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (downlink_available && downlink_len > 0) {
            memcpy(buf, downlink_buf, downlink_len);
            *len               = downlink_len;
            downlink_available = false;
            got                = true;
        }
        xSemaphoreGive(xDownlinkMutex);
    }
    return got;
}

void vLoRaWanTask(void *pvParameters)
{
    LOG("[No2-LoRa] Inicializando (ABP, AU915)...\n");

    xDownlinkMutex = xSemaphoreCreateMutex();
    if (!xDownlinkMutex) { LOG("[No2-LoRa] ERRO: mutex.\n"); vTaskDelete(NULL); return; }

    lorawan_debug(true);

    if (lorawan_init_abp(&sx1276_settings, LORAMAC_REGION_AU915, &abp_settings) != 0) {
        LOG("[No2-LoRa] ERRO FATAL: init ABP.\n");
        vTaskDelete(NULL); return;
    }

    lorawan_join();
    for (uint32_t t = 0; !lorawan_is_joined() && t < 100; t++) {
        lorawan_process();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (!lorawan_is_joined()) {
        LOG("[No2-LoRa] ERRO: Join nao confirmado.\n");
        vTaskDelete(NULL); return;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    LOG("[No2-LoRa] Join OK.\n");
    is_joined = true;
    if (xLoRaInitSemaphore) xSemaphoreGive(xLoRaInitSemaphore);

    uint8_t rx_buf[64];
    uint8_t rx_port = 0;
    while (1) {
        lorawan_process();

        int rx_len = lorawan_receive(rx_buf, sizeof(rx_buf), &rx_port);
        if (rx_len > 0) {
            LOG("[No2-LoRa] Downlink: porta=%d len=%d\n", rx_port, rx_len);
            if (xSemaphoreTake(xDownlinkMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                memcpy(downlink_buf, rx_buf, rx_len);
                downlink_len       = (uint8_t)rx_len;
                downlink_available = true;
                xSemaphoreGive(xDownlinkMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
