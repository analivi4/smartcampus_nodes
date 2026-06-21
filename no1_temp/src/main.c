/*
 * main.c — Nó 1: Temperatura/Umidade (DHT11) + Controle IR raw (AC)
 * UFC Quixadá Smart Campus
 *
 * Payload uplink  (porta 1): "T|XX.X\n"
 * Payload downlink (1 byte):
 *   0x31 → ligar AC   (IR_CMD_ON)
 *   0x30 → desligar AC (IR_CMD_OFF)
 *
 * Pinos:
 *   DHT11 DATA = GPIO 22  |  IR TX = GPIO 3  |  LED = GPIO 25
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
#include "dht11.h"
#include "ir_raw.h"
#include "ir_ac_commands.h"

#define LED_PIN    25
#define DHT11_PIN  22
#define IR_TX_PIN   3

uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((aligned(8)));
SemaphoreHandle_t xLoRaInitSemaphore = NULL;
SemaphoreHandle_t xRadioBusyMutex    = NULL;

/* =========================================================================
 * Task: Sensor
 * ========================================================================= */
static void vSensorTask(void *pvParameters)
{
    LOG("[No1-Sensor] Aguardando join LoRa...\n");
    xSemaphoreTake(xLoRaInitSemaphore, portMAX_DELAY);
    LOG("[No1-Sensor] Iniciando leituras.\n");

    dht11_result_t reading;
    char payload[100];

    while (1) {
        LOG("[No1-Sensor] Lendo DHT11...\n");
        dht11_status_t status = dht11_read(DHT11_PIN, &reading);

        if (status == DHT11_OK) {
            snprintf(payload, sizeof(payload), "T|%.1f\n", reading.temperature);
            LOG("[No1-Sensor] %s\n", payload);
            LoRaWan_Send((uint8_t *)payload, strlen(payload));

            gpio_put(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN, 0);
        } else {
            LOG("[No1-Sensor] DHT11: %s\n", dht11_status_str(status));
        }

        /* --- Verifica downlink (comando IR para o AC) --- */
        uint8_t dl_buf[64];
        uint8_t dl_len = 0;
        if (LoRaWan_GetDownlink(dl_buf, &dl_len) && dl_len >= 1) {
            LOG("[No1-Sensor] Downlink: 0x%02X\n", dl_buf[0]);

            if (dl_buf[0] == 0x31) {
                LOG("[No1-Sensor] Enviando IR: LIGAR AC\n");
                ir_raw_send(IR_TX_PIN, IR_CMD_ON, IR_CMD_LENGTH);
            } else if (dl_buf[0] == 0x30) {
                LOG("[No1-Sensor] Enviando IR: DESLIGAR AC\n");
                ir_raw_send(IR_TX_PIN, IR_CMD_OFF, IR_CMD_LENGTH);
            } else {
                LOG("[No1-Sensor] Downlink desconhecido: 0x%02X\n", dl_buf[0]);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(60000));
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

    LOG("\n=== UFC Quixada Smart Campus — No 1: Temp/Hum + IR AC ===\n");

    dht11_init(DHT11_PIN);
    ir_raw_init(IR_TX_PIN);

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
    xTaskCreate(vSensorTask,  "Sensor",    2048, NULL, 2, NULL);

    vTaskStartScheduler();
    while (1) { LOG("[ERRO] Scheduler!\n"); sleep_ms(1000); }
    return 0;
}
