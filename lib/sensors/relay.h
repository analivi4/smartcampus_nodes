/*
 * relay.h — Controle de relay via GPIO
 * UFC Quixadá Smart Campus — Nó 2
 */

#ifndef RELAY_H
#define RELAY_H

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

void relay_init(uint8_t pin);
void relay_set(uint8_t pin, bool on);
bool relay_get(uint8_t pin);
void relay_toggle(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* RELAY_H */
