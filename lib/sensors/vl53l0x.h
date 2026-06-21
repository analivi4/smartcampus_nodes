/*
 * vl53l0x.h — Driver VL53L0X via I2C (C puro)
 * UFC Quixadá Smart Campus — Nó 3
 *
 * Baseado no driver pololu/vl53l0x-arduino portado para Pico (C++),
 * reescrito em C puro para compatibilidade com o restante do projeto.
 *
 * Interface pública idêntica à versão anterior — nenhuma mudança
 * necessária em main.c ou no CMakeLists.
 */

#ifndef VL53L0X_H
#define VL53L0X_H

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
#include "hardware/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VL53L0X_ADDR_A       0x29
#define VL53L0X_ADDR_B       0x30
#define VL53L0X_OUT_OF_RANGE 65535  /* valor retornado em erro/timeout */

/* Registradores (subset usado) */
#define VL53_SYSRANGE_START                   0x00
#define VL53_SYSTEM_SEQUENCE_CONFIG           0x01
#define VL53_SYSTEM_INTERRUPT_CONFIG_GPIO     0x0A
#define VL53_SYSTEM_INTERRUPT_CLEAR           0x0B
#define VL53_RESULT_INTERRUPT_STATUS          0x13
#define VL53_RESULT_RANGE_STATUS              0x14
#define VL53_I2C_SLAVE_DEVICE_ADDRESS         0x8A
#define VL53_MSRC_CONFIG_CONTROL              0x60
#define VL53_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define VL53_SYSTEM_SEQUENCE_CONFIG           0x01
#define VL53_GPIO_HV_MUX_ACTIVE_HIGH          0x84
#define VL53_IDENTIFICATION_MODEL_ID          0xC0
#define VL53_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV 0x89
#define VL53_GLOBAL_CONFIG_SPAD_ENABLES_REF_0 0xB0
#define VL53_GLOBAL_CONFIG_REF_EN_START_SELECT 0xB6
#define VL53_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD 0x4E
#define VL53_DYNAMIC_SPAD_REF_EN_START_OFFSET 0x4F
#define VL53_PRE_RANGE_CONFIG_VCSEL_PERIOD    0x50
#define VL53_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI 0x51
#define VL53_FINAL_RANGE_CONFIG_VCSEL_PERIOD  0x70
#define VL53_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI 0x71
#define VL53_MSRC_CONFIG_TIMEOUT_MACROP       0x46
#define VL53_OSC_CALIBRATE_VAL                0xF8
#define VL53_ALGO_PHASECAL_CONFIG_TIMEOUT     0x30

/*
 * Estado interno de um sensor.
 * Alocado pelo chamador — permite dois sensores independentes.
 */
typedef struct {
    i2c_inst_t *i2c;
    uint8_t     addr;
    uint8_t     stop_variable;   /* salvo no init, restaurado antes de cada medição */
    uint32_t    timing_budget_us;
    uint16_t    io_timeout;
    uint32_t    timeout_start_ms;
    bool        did_timeout;
} vl53l0x_t;

/*
 * Inicializa um sensor.
 * Executa DataInit + StaticInit + RefCalibration (sequência completa pololu).
 * Retorna true em sucesso.
 */
bool vl53l0x_init(vl53l0x_t *dev, i2c_inst_t *i2c, uint8_t addr, uint16_t timeout_ms);

/*
 * Inicializa dois sensores no mesmo barramento com endereços diferentes.
 * Usa XSHUT para sequenciar a reprogramação de endereço do sensor B.
 * Retorna true se ambos inicializaram com sucesso.
 */
bool vl53l0x_init_dual(vl53l0x_t *dev_a, vl53l0x_t *dev_b,
                        i2c_inst_t *i2c,
                        uint8_t xshut_a, uint8_t xshut_b,
                        uint16_t timeout_ms);

/*
 * Dispara uma medição single-shot e retorna a distância em mm.
 * Retorna VL53L0X_OUT_OF_RANGE em caso de timeout ou erro.
 */
uint16_t vl53l0x_read_single_mm(vl53l0x_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* VL53L0X_H */