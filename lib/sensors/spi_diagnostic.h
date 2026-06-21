/*
 * spi_diagnostic.h — Diagnóstico SPI/SX1276
 * UFC Quixadá Smart Campus
 *
 * Baseado em spi_diagnostic.c do repositório Diogoassun/Pico_LoRaWAN.
 * Testa a comunicação SPI com o SX1276 ANTES de subir o scheduler FreeRTOS,
 * garantindo que o hardware está acessível e evitando falhas silenciosas.
 *
 * Uso:
 *   1. Chamar spi_diagnostic_init() com os pinos do seu nó.
 *   2. Chamar spi_diagnostic_run() — imprime resultado no serial e retorna bool.
 *   3. Se retornar false, travar antes de criar tasks (hardware com defeito).
 */

#ifndef SPI_DIAGNOSTIC_H
#define SPI_DIAGNOSTIC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef LOG
#  ifdef DEBUG
#    define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#  else
#    define LOG(fmt, ...) ((void)0)
#  endif
#endif
#include "hardware/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configuração de pinos passada para o diagnóstico */
typedef struct {
    spi_inst_t *spi_inst;  /* ex: spi0                */
    uint8_t     sck;       /* pino SCK                */
    uint8_t     mosi;      /* pino MOSI               */
    uint8_t     miso;      /* pino MISO               */
    uint8_t     nss;       /* pino NSS (chip select)  */
    uint8_t     reset;     /* pino RESET              */
    uint8_t     dio0;      /* pino DIO0 (verificação) */
} spi_diag_config_t;

/*
 * Inicializa SPI e GPIOs para o diagnóstico.
 * Deve ser chamada ANTES de spi_diagnostic_run().
 * Não inicializa o FreeRTOS — usa sleep_ms/sleep_us direto.
 */
void spi_diagnostic_init(const spi_diag_config_t *cfg);

/*
 * Executa a sequência completa de diagnóstico (idêntica ao Diogoassun):
 *   1. Verifica NSS e SPI
 *   2. Executa reset físico do rádio
 *   3. Lê VERSION (0x42) — 3 tentativas; esperado 0x12
 *   4. Teste de escrita/leitura em RegOpMode
 *   5. Verifica estado do DIO0
 *   6. Testa velocidade a 8MHz
 *
 * Imprime resultado detalhado via printf (USB Serial / UART).
 * Retorna true se o rádio foi detectado corretamente (VERSION == 0x12).
 */
bool spi_diagnostic_run(const spi_diag_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* SPI_DIAGNOSTIC_H */
