/*
 * dht11.c — Biblioteca do sensor DHT11 OTIMIZADA para FreeRTOS
 * UFC Quixadá Smart Campus — Nó 1
 */

#include "dht11.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

__attribute__((weak)) SemaphoreHandle_t xRadioBusyMutex = NULL;

#define EDGE_TIMEOUT_US 1000

void dht11_init(uint8_t pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

static inline bool _wait_for_level(uint8_t pin, bool level, uint32_t timeout_us)
{
    uint32_t t = 0;
    while (gpio_get(pin) != level) {
        busy_wait_us(1);
        if (++t > timeout_us) return false;
    }
    return true;
}

dht11_status_t dht11_read(uint8_t pin, dht11_result_t *result)
{
    uint8_t data[5] = {0};

    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);

    /* O processador fica livre para processar a rádio LoRa durante estes 20ms */
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_put(pin, 1);
    busy_wait_us(30);
    gpio_set_dir(pin, GPIO_IN);

    /* --- FASE CRÍTICA: Desativa mudanças de contexto do FreeRTOS --- */
    if (xRadioBusyMutex) xSemaphoreTake(xRadioBusyMutex, portMAX_DELAY);
    taskENTER_CRITICAL();

    if (!_wait_for_level(pin, 0, EDGE_TIMEOUT_US)) {
        taskEXIT_CRITICAL();
        if (xRadioBusyMutex) xSemaphoreGive(xRadioBusyMutex);
        return DHT11_ERR_NO_RESPONSE;
    }
    if (!_wait_for_level(pin, 1, EDGE_TIMEOUT_US)) {
        taskEXIT_CRITICAL();
        if (xRadioBusyMutex) xSemaphoreGive(xRadioBusyMutex);
        return DHT11_ERR_NO_RESPONSE;
    }
    if (!_wait_for_level(pin, 0, EDGE_TIMEOUT_US)) {
        taskEXIT_CRITICAL();
        if (xRadioBusyMutex) xSemaphoreGive(xRadioBusyMutex);
        return DHT11_ERR_NO_RESPONSE;
    }

    /* Lê 40 bits */
    for (int i = 0; i < 40; i++) {
        if (!_wait_for_level(pin, 1, EDGE_TIMEOUT_US)) {
            taskEXIT_CRITICAL();
            if (xRadioBusyMutex) xSemaphoreGive(xRadioBusyMutex);
            return DHT11_ERR_TIMEOUT;
        }

        busy_wait_us(40);

        data[i / 8] <<= 1;
        if (gpio_get(pin)) {
            data[i / 8] |= 1;
            if (!_wait_for_level(pin, 0, EDGE_TIMEOUT_US)) {
                taskEXIT_CRITICAL();
                if (xRadioBusyMutex) xSemaphoreGive(xRadioBusyMutex);
                return DHT11_ERR_TIMEOUT;
            }
        }
    }

    /* Fim da zona crítica */
    taskEXIT_CRITICAL();
    if (xRadioBusyMutex) xSemaphoreGive(xRadioBusyMutex);

    /* --- Checksum --- */
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        LOG("[DHT11] Checksum: calc=0x%02X recv=0x%02X\n", checksum, data[4]);
        return DHT11_ERR_CHECKSUM;
    }

    result->humidity    = (float)data[0] + (float)data[1] / 10.0f;
    result->temperature = (float)data[2] + (float)data[3] / 10.0f;
    return DHT11_OK;
}

const char *dht11_status_str(dht11_status_t status)
{
    switch (status) {
        case DHT11_OK:              return "OK";
        case DHT11_ERR_NO_RESPONSE: return "Sem resposta";
        case DHT11_ERR_CHECKSUM:    return "Checksum erro";
        case DHT11_ERR_TIMEOUT:     return "Timeout";
        default:                    return "Desconhecido";
    }
}
