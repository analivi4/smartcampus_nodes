/*
 * ir_raw.h — Transmissor IR por raw timing (protocolo proprietário de AC)
 * UFC Quixadá Smart Campus — Nó 1
 *
 * Reproduz sequências de pulsos capturadas do controle remoto original.
 * Os dados são arrays de uint16_t onde cada posição alterna entre:
 *   índice par  → duração do BURST (µs) com carrier 38kHz
 *   índice ímpar → duração do ESPAÇO (µs) sem sinal
 *
 * Uso:
 *   ir_raw_init(IR_TX_PIN);
 *   ir_raw_send(IR_TX_PIN, rawDataOFF, 197);  // desligar AC
 *   ir_raw_send(IR_TX_PIN, rawDataON,  197);  // ligar AC
 */

#ifndef IR_RAW_H
#define IR_RAW_H

#include <stdint.h>
#include <stddef.h>
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

/*
 * Inicializa o GPIO do LED IR como saída (começa em LOW).
 */
void ir_raw_init(uint8_t pin);

/*
 * Transmite uma sequência raw de pulsos IR.
 *
 * pin      : GPIO do LED IR (via transistor)
 * raw_data : array uint16_t com durações em µs, alternando burst/espaço
 * length   : número de elementos do array (ex: 197)
 *
 * O carrier usado é 38kHz (período ~26µs, duty cycle 50%).
 * A função bloqueia até o frame completo ser transmitido.
 */
void ir_raw_send(uint8_t pin, const uint16_t *raw_data, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* IR_RAW_H */
