/*
 * dht11.h — Biblioteca do sensor DHT11
 * UFC Quixadá Smart Campus — Nó 1
 *
 * Leitura bit-bang single-wire.
 * Deve ser chamada com interrupções desabilitadas para timing preciso:
 *
 *   dht11_result_t r;
 *   uint32_t irq = save_and_disable_interrupts();
 *   dht11_status_t s = dht11_read(DHT11_PIN, &r);
 *   restore_interrupts(irq);
 */

#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef LOG
#  ifdef DEBUG
#    define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#  else
#    define LOG(fmt, ...) ((void)0)
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DHT11_OK = 0,
    DHT11_ERR_NO_RESPONSE, /* sensor não respondeu ao sinal de start */
    DHT11_ERR_CHECKSUM,    /* dados corrompidos                      */
    DHT11_ERR_TIMEOUT,     /* timeout lendo bits                     */
} dht11_status_t;

typedef struct {
    float temperature; /* °C */
    float humidity;    /* %RH */
} dht11_result_t;

/*
 * Inicializa o pino GPIO para uso com o DHT11.
 * Deve ser chamada uma vez antes de dht11_read().
 */
void dht11_init(uint8_t pin);

/*
 * Lê temperatura e umidade do DHT11.
 *
 * ATENÇÃO: desabilite interrupções antes de chamar:
 *   uint32_t irq = save_and_disable_interrupts();
 *   dht11_status_t s = dht11_read(pin, &result);
 *   restore_interrupts(irq);
 *
 * Retorna DHT11_OK em caso de sucesso.
 * Em caso de erro, result não é modificado.
 */
dht11_status_t dht11_read(uint8_t pin, dht11_result_t *result);

/* Retorna string legível do status (para debug) */
const char *dht11_status_str(dht11_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* DHT11_H */
