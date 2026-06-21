/*
 * main.c — Nó 3: Contagem Bidirecional de Pessoas (VL53L0X × 2)
 * UFC Quixadá Smart Campus
 *
 * Payload uplink (porta 1): "O|NN\n"
 * Pinos: SDA=4, SCL=5, XSHUT_A=6, XSHUT_B=7, LED=25
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/i2c.h"
#include "pico/lorawan.h"

#include "lorawan_task.h"
#include "spi_diagnostic.h"
#include "vl53l0x.h"

#define LED_PIN         25
#define I2C_SDA          4
#define I2C_SCL          5
#define XSHUT_A          6
#define XSHUT_B          7
#define PRESENCE_MM      500
#define PASS_TIMEOUT_MS  3000
#define HEARTBEAT_MS     60000

uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((aligned(8)));
SemaphoreHandle_t xLoRaInitSemaphore = NULL;

static vl53l0x_t sensor_a;
static vl53l0x_t sensor_b;

static void vOccupancyTask(void *pvParameters)
{
    LOG("[No3] Aguardando join LoRa...\n");
    xSemaphoreTake(xLoRaInitSemaphore, portMAX_DELAY);
    LOG("[No3] Iniciando contagem.\n");

    typedef enum { IDLE, A_FIRST, B_FIRST } DetState;

    int      people_count   = 0;
    int      last_sent      = -1;
    uint32_t last_send_ms   = 0;
    DetState state          = IDLE;
    uint32_t state_start_ms = 0;
    char     payload[16];

    while (1) {
        uint16_t da = vl53l0x_read_single_mm(&sensor_a);
        uint16_t db = vl53l0x_read_single_mm(&sensor_b);

        bool ta = (da < PRESENCE_MM && da != VL53L0X_OUT_OF_RANGE);
        bool tb = (db < PRESENCE_MM && db != VL53L0X_OUT_OF_RANGE);
        uint32_t now = to_ms_since_boot(get_absolute_time());

        switch (state) {
            case IDLE:
                if      (ta && !tb) { state = A_FIRST; state_start_ms = now; }
                else if (tb && !ta) { state = B_FIRST; state_start_ms = now; }
                break;
            case A_FIRST:
                if (tb) { people_count++; LOG("[No3] ENTRADA. Total: %d\n", people_count); state = IDLE; }
                else if ((now - state_start_ms) > PASS_TIMEOUT_MS) { state = IDLE; }
                break;
            case B_FIRST:
                if (ta) { if (people_count > 0) people_count--; LOG("[No3] SAIDA. Total: %d\n", people_count); state = IDLE; }
                else if ((now - state_start_ms) > PASS_TIMEOUT_MS) { state = IDLE; }
                break;
        }

        if (people_count != last_sent || (now - last_send_ms) >= HEARTBEAT_MS) {
            snprintf(payload, sizeof(payload), "O|%d\n", people_count);
            LOG("[No3] Uplink: %s\n", payload);
            LoRaWan_Send((uint8_t *)payload, strlen(payload));
            last_sent    = people_count;
            last_send_ms = now;
            gpio_put(LED_PIN, 1); vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);

    lorawan_erase_nvm();
    sleep_ms(100);

    gpio_init(LED_PIN); gpio_set_dir(LED_PIN, GPIO_OUT); gpio_put(LED_PIN, 1);

    LOG("\n=== UFC Quixada Smart Campus — No 3: Contagem de Pessoas ===\n");

    i2c_init(i2c0, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    if (!vl53l0x_init_dual(&sensor_a, &sensor_b, i2c0, XSHUT_A, XSHUT_B, 500)) {
        LOG("[AVISO] Falha em um ou mais sensores VL53L0X.\n");
    }

    static const spi_diag_config_t diag_cfg = {
        .spi_inst = spi0, .sck = 18, .mosi = 19, .miso = 16,
        .nss = 17, .reset = 15, .dio0 = 20,
    };
    spi_diagnostic_init(&diag_cfg);
    if (!spi_diagnostic_run(&diag_cfg)) {
        LOG("[ERRO FATAL] Radio nao detectado.\n");
        while (1) { gpio_put(LED_PIN, 1); sleep_ms(100); gpio_put(LED_PIN, 0); sleep_ms(100); }
    }

    xLoRaInitSemaphore = xSemaphoreCreateBinary();
    if (!xLoRaInitSemaphore) { LOG("[ERRO] Semaforo.\n"); return -1; }

    xTaskCreate(vLoRaWanTask,   "LoRaStack", 8192, NULL, 4, NULL);
    xTaskCreate(vOccupancyTask, "Occupancy", 2048, NULL, 2, NULL);
    vTaskStartScheduler();
    while (1);
}
