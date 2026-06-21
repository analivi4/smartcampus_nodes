#ifndef LORAWAN_TASK_H
#define LORAWAN_TASK_H

#include <stdint.h>
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

void vLoRaWanTask(void *pvParameters);
void LoRaWan_Send(uint8_t *data, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif
