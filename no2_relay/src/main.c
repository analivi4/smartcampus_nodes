/*
 * main.c — Nó 2: Estado do Relay (uplink) + Controle Remoto (downlink)
 * UFC Quixadá Smart Campus
 *
 * Payload uplink  (porta 1): "R|0\n" ou "R|1\n"
 * Payload downlink (1 byte): 0x31 = ligar | 0x30 = desligar
 *
 * Pinos:
 *   RELAY = GPIO 2  |  LED = GPIO 25
 *   LoRa: NSS=17, SCK=18, MOSI=19, MISO=16, RST=15, DIO0=20, DIO1=21
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/lorawan.h"

#include "lorawan_task.h"
#include "spi_diagnostic.h"
#include "relay.h"

#define LED_PIN    25
#define RELAY_PIN   2

uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((aligned(8)));
SemaphoreHandle_t xLoRaInitSemaphore = NULL;

/* =========================================================================
 * Task: Relay
 * ========================================================================= */
static void vRelayTask(void *pvParameters)
{
    LOG("[No2-Relay] Aguardando join LoRa...\n");
    xSemaphoreTake(xLoRaInitSemaphore, portMAX_DELAY);
    LOG("[No2-Relay] Iniciando.\n");

    char payload[16];

    while (1) {
        /* --- Uplink: reporta estado atual --- */
        bool state = relay_get(RELAY_PIN);
        snprintf(payload, sizeof(payload), "R|%d\n", state ? 1 : 0);
        LOG("[No2-Relay] Uplink: %s\n", payload);
        LoRaWan_Send((uint8_t *)payload, strlen(payload));

        gpio_put(LED_PIN, 1); vTaskDelay(pdMS_TO_TICKS(100));
        gpio_put(LED_PIN, 0);

        /* --- Verifica downlink --- */
        uint8_t dl_buf[64];
        uint8_t dl_len = 0;
        if (LoRaWan_GetDownlink(dl_buf, &dl_len) && dl_len >= 1) {
            LOG("[No2-Relay] Downlink: 0x%02X\n", dl_buf[0]);
            relay_set(RELAY_PIN, dl_buf[0] == 0x31);

            /* Envia uplink imediato confirmando o acionamento */
            state = relay_get(RELAY_PIN);
            snprintf(payload, sizeof(payload), "R|%d\n", state ? 1 : 0);
            LoRaWan_Send((uint8_t *)payload, strlen(payload));
        }

        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(void)
{
    stdio_init_all();
    sleep_ms(2000);

    lorawan_erase_nvm();
    sleep_ms(100);

    gpio_init(LED_PIN); gpio_set_dir(LED_PIN, GPIO_OUT); gpio_put(LED_PIN, 1);

    LOG("\n=== UFC Quixada Smart Campus — No 2: Relay + Downlink ===\n");

    relay_init(RELAY_PIN);

    static const spi_diag_config_t diag_cfg = {
        .spi_inst = spi0,
        .sck = 18, .mosi = 19, .miso = 16,
        .nss = 17, .reset = 15, .dio0 = 20,
    };
    spi_diagnostic_init(&diag_cfg);
    if (!spi_diagnostic_run(&diag_cfg)) {
        LOG("[ERRO FATAL] Radio nao detectado.\n");
        while (1) { gpio_put(LED_PIN, 1); sleep_ms(100); gpio_put(LED_PIN, 0); sleep_ms(100); }
    }

    xLoRaInitSemaphore = xSemaphoreCreateBinary();
    if (!xLoRaInitSemaphore) { LOG("[ERRO] Semaforo.\n"); return -1; }

    xTaskCreate(vLoRaWanTask, "LoRaStack", 8192, NULL, 4, NULL);
    xTaskCreate(vRelayTask,   "Relay",     2048, NULL, 2, NULL);

    vTaskStartScheduler();
    while (1) { LOG("[ERRO] Scheduler!\n"); sleep_ms(1000); }
    return 0;
}
