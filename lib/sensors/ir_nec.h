/*
 * ir_nec.h — Transmissor IR protocolo NEC
 * UFC Quixadá Smart Campus — Nó 1
 *
 * Envia comandos NEC via LED IR (GPIO + transistor).
 * Carrier de 38kHz gerado por bit-bang.
 *
 * Uso:
 *   ir_nec_init(IR_TX_PIN);
 *   ir_nec_send(IR_TX_PIN, 0xB2, 0x01);  // address=0xB2, command=0x01
 */

#ifndef IR_NEC_H
#define IR_NEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Inicializa o GPIO do LED IR como saída.
 */
void ir_nec_init(uint8_t pin);

/*
 * Envia um frame NEC completo:
 *   - Leader: 9ms burst + 4.5ms espaço
 *   - address + ~address + command + ~command  (LSB first)
 *   - Stop bit
 *
 * address: endereço do dispositivo (ex: 0xB2 para alguns modelos de AC)
 * command: código do comando      (ex: 0x01=ligar, 0x00=desligar)
 *
 * NOTA: Este é o protocolo NEC padrão de 32 bits.
 * Ar-condicionados de alguns fabricantes usam extensões proprietárias
 * com frames maiores — ajuste conforme o seu modelo.
 */
void ir_nec_send(uint8_t pin, uint8_t address, uint8_t command);

#ifdef __cplusplus
}
#endif

#endif /* IR_NEC_H */
