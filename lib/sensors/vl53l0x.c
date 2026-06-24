/*
 * vl53l0x.c — Driver VL53L0X via I2C (C puro)
 * UFC Quixadá Smart Campus — Nó 3
 *
 * Baseado em pololu/vl53l0x-arduino (C++) portado para Pico C++,
 * reescrito em C puro. Lógica e sequências de registradores preservadas.
 */

#include "vl53l0x.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Macros internos (do pololu)
 * -------------------------------------------------------------------------- */
#define decodeVcselPeriod(r)    (((r) + 1) << 1)
#define encodeVcselPeriod(p)    (((p) >> 1) - 1)
#define calcMacroPeriod(p)      ((((uint32_t)2304 * (p) * 1655) + 500) / 1000)

/* --------------------------------------------------------------------------
 * I2C helpers
 * -------------------------------------------------------------------------- */

static void _wr(vl53l0x_t *d, uint8_t reg, uint8_t val)
{
    uint8_t b[2] = {reg, val};
    i2c_write_blocking(d->i2c, d->addr, b, 2, false);
}

static void _wr16(vl53l0x_t *d, uint8_t reg, uint16_t val)
{
    uint8_t b[3] = {reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};
    i2c_write_blocking(d->i2c, d->addr, b, 3, false);
}

static void _wr32(vl53l0x_t *d, uint8_t reg, uint32_t val)
{
    uint8_t b[5] = {reg,
        (uint8_t)(val >> 24), (uint8_t)(val >> 16),
        (uint8_t)(val >> 8),  (uint8_t)(val & 0xFF)};
    i2c_write_blocking(d->i2c, d->addr, b, 5, false);
}

static uint8_t _rd(vl53l0x_t *d, uint8_t reg)
{
    uint8_t val = 0;
    i2c_write_blocking(d->i2c, d->addr, &reg, 1, true);
    i2c_read_blocking(d->i2c, d->addr, &val, 1, false);
    return val;
}

static uint16_t _rd16(vl53l0x_t *d, uint8_t reg)
{
    uint8_t b[2] = {0};
    i2c_write_blocking(d->i2c, d->addr, &reg, 1, true);
    i2c_read_blocking(d->i2c, d->addr, b, 2, false);
    return ((uint16_t)b[0] << 8) | b[1];
}

static void _rdm(vl53l0x_t *d, uint8_t reg, uint8_t *dst, uint8_t n)
{
    i2c_write_blocking(d->i2c, d->addr, &reg, 1, true);
    i2c_read_blocking(d->i2c, d->addr, dst, n, false);
}

static void _wrm(vl53l0x_t *d, uint8_t reg, const uint8_t *src, uint8_t n)
{
    uint8_t buf[n + 1];
    buf[0] = reg;
    memcpy(buf + 1, src, n);
    i2c_write_blocking(d->i2c, d->addr, buf, n + 1, false);
}

/* --------------------------------------------------------------------------
 * Timeout helpers
 * -------------------------------------------------------------------------- */

static void _timeout_start(vl53l0x_t *d)
{
    d->timeout_start_ms = to_ms_since_boot(get_absolute_time());
}

static bool _timeout_expired(vl53l0x_t *d)
{
    return d->io_timeout > 0 &&
           (to_ms_since_boot(get_absolute_time()) - d->timeout_start_ms > d->io_timeout);
}

/* --------------------------------------------------------------------------
 * Internos — sequência pololu
 * -------------------------------------------------------------------------- */

static uint16_t _decode_timeout(uint16_t v)
{
    return (uint16_t)((v & 0x00FF) << (uint16_t)((v & 0xFF00) >> 8)) + 1;
}

static uint16_t _encode_timeout(uint32_t mclks)
{
    if (mclks == 0) return 0;
    uint32_t ls = mclks - 1;
    uint16_t ms = 0;
    while ((ls & 0xFFFFFF00) > 0) { ls >>= 1; ms++; }
    return (ms << 8) | (ls & 0xFF);
}

static uint32_t _mclks_to_us(uint16_t mclks, uint8_t vcsel)
{
    uint32_t mp = calcMacroPeriod(vcsel);
    return ((mclks * mp) + 500) / 1000;
}

static uint32_t _us_to_mclks(uint32_t us, uint8_t vcsel)
{
    uint32_t mp = calcMacroPeriod(vcsel);
    return (((us * 1000) + (mp / 2)) / mp);
}

typedef struct {
    bool tcc, msrc, dss, pre_range, final_range;
} _enables_t;

typedef struct {
    uint16_t pre_range_vcsel_pclks, final_range_vcsel_pclks;
    uint16_t msrc_mclks, pre_range_mclks, final_range_mclks;
    uint32_t msrc_us,    pre_range_us,    final_range_us;
} _timeouts_t;

static void _get_enables(vl53l0x_t *d, _enables_t *e)
{
    uint8_t sc = _rd(d, VL53_SYSTEM_SEQUENCE_CONFIG);
    e->tcc        = (sc >> 4) & 1;
    e->dss        = (sc >> 3) & 1;
    e->msrc       = (sc >> 2) & 1;
    e->pre_range  = (sc >> 6) & 1;
    e->final_range= (sc >> 7) & 1;
}

static void _get_timeouts(vl53l0x_t *d, _enables_t *e, _timeouts_t *t)
{
    t->pre_range_vcsel_pclks   = decodeVcselPeriod(_rd(d, VL53_PRE_RANGE_CONFIG_VCSEL_PERIOD));
    t->msrc_mclks              = _rd(d, VL53_MSRC_CONFIG_TIMEOUT_MACROP) + 1;
    t->msrc_us                 = _mclks_to_us(t->msrc_mclks, t->pre_range_vcsel_pclks);
    t->pre_range_mclks         = _decode_timeout(_rd16(d, VL53_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
    t->pre_range_us            = _mclks_to_us(t->pre_range_mclks, t->pre_range_vcsel_pclks);
    t->final_range_vcsel_pclks = decodeVcselPeriod(_rd(d, VL53_FINAL_RANGE_CONFIG_VCSEL_PERIOD));
    t->final_range_mclks       = _decode_timeout(_rd16(d, VL53_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));
    if (e->pre_range) t->final_range_mclks -= t->pre_range_mclks;
    t->final_range_us          = _mclks_to_us(t->final_range_mclks, t->final_range_vcsel_pclks);
}

static uint32_t _get_timing_budget(vl53l0x_t *d)
{
    _enables_t e; _timeouts_t t;
    _get_enables(d, &e); _get_timeouts(d, &e, &t);
    uint32_t us = 1910 + 960;
    if (e.tcc)         us += t.msrc_us + 590;
    if (e.dss)         us += 2 * (t.msrc_us + 690);
    else if (e.msrc)   us += t.msrc_us + 660;
    if (e.pre_range)   us += t.pre_range_us + 660;
    if (e.final_range) us += t.final_range_us + 550;
    return us;
}

static bool _set_timing_budget(vl53l0x_t *d, uint32_t budget_us)
{
    _enables_t e; _timeouts_t t;
    _get_enables(d, &e); _get_timeouts(d, &e, &t);
    uint32_t used = 1910 + 960;
    if (e.tcc)       used += t.msrc_us + 590;
    if (e.dss)       used += 2*(t.msrc_us + 690);
    else if (e.msrc) used += t.msrc_us + 660;
    if (e.pre_range) used += t.pre_range_us + 660;
    if (e.final_range) {
        used += 550;
        if (used > budget_us) return false;
        uint32_t fr_mclks = _us_to_mclks(budget_us - used, t.final_range_vcsel_pclks);
        if (e.pre_range) fr_mclks += t.pre_range_mclks;
        _wr16(d, VL53_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, _encode_timeout(fr_mclks));
        d->timing_budget_us = budget_us;
    }
    return true;
}

static bool _get_spad_info(vl53l0x_t *d, uint8_t *count, bool *is_aperture)
{
    _wr(d, 0x80, 0x01); _wr(d, 0xFF, 0x01); _wr(d, 0x00, 0x00);
    _wr(d, 0xFF, 0x06);
    _wr(d, 0x83, _rd(d, 0x83) | 0x04);
    _wr(d, 0xFF, 0x07); _wr(d, 0x81, 0x01); _wr(d, 0x80, 0x01);
    _wr(d, 0x94, 0x6B); _wr(d, 0x83, 0x00);
    _timeout_start(d);
    while (_rd(d, 0x83) == 0x00) {
        if (_timeout_expired(d)) return false;
    }
    _wr(d, 0x83, 0x01);
    uint8_t tmp = _rd(d, 0x92);
    *count       = tmp & 0x7F;
    *is_aperture = (tmp >> 7) & 0x01;
    _wr(d, 0x81, 0x00); _wr(d, 0xFF, 0x06);
    _wr(d, 0x83, _rd(d, 0x83) & ~0x04);
    _wr(d, 0xFF, 0x01); _wr(d, 0x00, 0x01);
    _wr(d, 0xFF, 0x00); _wr(d, 0x80, 0x00);
    return true;
}

static bool _single_ref_cal(vl53l0x_t *d, uint8_t vhv_init)
{
    _wr(d, VL53_SYSRANGE_START, 0x01 | vhv_init);
    _timeout_start(d);
    while ((_rd(d, VL53_RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if (_timeout_expired(d)) return false;
    }
    _wr(d, VL53_SYSTEM_INTERRUPT_CLEAR, 0x01);
    _wr(d, VL53_SYSRANGE_START, 0x00);
    return true;
}

/* --------------------------------------------------------------------------
 * Init completo — DataInit + StaticInit + RefCalibration (pololu)
 * -------------------------------------------------------------------------- */

static bool _do_init(vl53l0x_t *d)
{
    sleep_ms(2);

    uint8_t model = _rd(d, VL53_IDENTIFICATION_MODEL_ID);
    LOG("[VL53] addr=0x%02X model_id=0x%02X (esperado 0xEE)\n", d->addr, model);
    if (model != 0xEE) return false;

    /* 2V8 mode */
    _wr(d, VL53_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV,
        _rd(d, VL53_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV) | 0x01);

    /* Lê stop_variable — CRÍTICO para single-shot */
    _wr(d, 0x88, 0x00);
    _wr(d, 0x80, 0x01); _wr(d, 0xFF, 0x01); _wr(d, 0x00, 0x00);
    d->stop_variable = _rd(d, 0x91);
    LOG("[VL53] stop_variable=0x%02X\n", d->stop_variable);
    _wr(d, 0x00, 0x01); _wr(d, 0xFF, 0x00); _wr(d, 0x80, 0x00);

    /* Desabilita limit checks desnecessários */
    _wr(d, VL53_MSRC_CONFIG_CONTROL, _rd(d, VL53_MSRC_CONFIG_CONTROL) | 0x12);

    _wr16(d, VL53_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, (uint16_t)(0.2f * (1 << 7)));

    _wr(d, VL53_SYSTEM_SEQUENCE_CONFIG, 0xFF);

    /* SPAD info */
    uint8_t spad_count; bool spad_aperture;
    if (!_get_spad_info(d, &spad_count, &spad_aperture)) return false;

    uint8_t ref_spad[6];
    _rdm(d, VL53_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad, 6);

    _wr(d, 0xFF, 0x01);
    _wr(d, VL53_DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
    _wr(d, VL53_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
    _wr(d, 0xFF, 0x00);
    _wr(d, VL53_GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

    uint8_t first = spad_aperture ? 12 : 0;
    uint8_t enabled = 0;
    for (uint8_t i = 0; i < 48; i++) {
        if (i < first || enabled == spad_count)
            ref_spad[i/8] &= ~(1 << (i%8));
        else if ((ref_spad[i/8] >> (i%8)) & 1)
            enabled++;
    }
    _wrm(d, VL53_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad, 6);

    /* Tuning settings */
    _wr(d, 0xFF, 0x01); _wr(d, 0x00, 0x00);
    _wr(d, 0xFF, 0x00); _wr(d, 0x09, 0x00); _wr(d, 0x10, 0x00); _wr(d, 0x11, 0x00);
    _wr(d, 0x24, 0x01); _wr(d, 0x25, 0xFF); _wr(d, 0x75, 0x00);
    _wr(d, 0xFF, 0x01); _wr(d, 0x4E, 0x2C); _wr(d, 0x48, 0x00); _wr(d, 0x30, 0x20);
    _wr(d, 0xFF, 0x00); _wr(d, 0x30, 0x09); _wr(d, 0x54, 0x00); _wr(d, 0x31, 0x04);
    _wr(d, 0x32, 0x03); _wr(d, 0x40, 0x83); _wr(d, 0x46, 0x25); _wr(d, 0x60, 0x00);
    _wr(d, 0x27, 0x00); _wr(d, 0x50, 0x06); _wr(d, 0x51, 0x00); _wr(d, 0x52, 0x96);
    _wr(d, 0x56, 0x08); _wr(d, 0x57, 0x30); _wr(d, 0x61, 0x00); _wr(d, 0x62, 0x00);
    _wr(d, 0x64, 0x00); _wr(d, 0x65, 0x00); _wr(d, 0x66, 0xA0);
    _wr(d, 0xFF, 0x01); _wr(d, 0x22, 0x32); _wr(d, 0x47, 0x14); _wr(d, 0x49, 0xFF);
    _wr(d, 0x4A, 0x00);
    _wr(d, 0xFF, 0x00); _wr(d, 0x7A, 0x0A); _wr(d, 0x7B, 0x00); _wr(d, 0x78, 0x21);
    _wr(d, 0xFF, 0x01); _wr(d, 0x23, 0x34); _wr(d, 0x42, 0x00); _wr(d, 0x44, 0xFF);
    _wr(d, 0x45, 0x26); _wr(d, 0x46, 0x05); _wr(d, 0x40, 0x40); _wr(d, 0x0E, 0x06);
    _wr(d, 0x20, 0x1A); _wr(d, 0x43, 0x40);
    _wr(d, 0xFF, 0x00); _wr(d, 0x34, 0x03); _wr(d, 0x35, 0x44);
    _wr(d, 0xFF, 0x01); _wr(d, 0x31, 0x04); _wr(d, 0x4B, 0x09); _wr(d, 0x4C, 0x05);
    _wr(d, 0x4D, 0x04);
    _wr(d, 0xFF, 0x00); _wr(d, 0x44, 0x00); _wr(d, 0x45, 0x20); _wr(d, 0x47, 0x08);
    _wr(d, 0x48, 0x28); _wr(d, 0x67, 0x00); _wr(d, 0x70, 0x04); _wr(d, 0x71, 0x01);
    _wr(d, 0x72, 0xFE); _wr(d, 0x76, 0x00); _wr(d, 0x77, 0x00);
    _wr(d, 0xFF, 0x01); _wr(d, 0x0D, 0x01);
    _wr(d, 0xFF, 0x00); _wr(d, 0x80, 0x01); _wr(d, 0x01, 0xF8);
    _wr(d, 0xFF, 0x01); _wr(d, 0x8E, 0x01); _wr(d, 0x00, 0x01);
    _wr(d, 0xFF, 0x00); _wr(d, 0x80, 0x00);

    /* Interrupt config */
    _wr(d, VL53_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
    _wr(d, VL53_GPIO_HV_MUX_ACTIVE_HIGH,
        _rd(d, VL53_GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);
    _wr(d, VL53_SYSTEM_INTERRUPT_CLEAR, 0x01);

    d->timing_budget_us = _get_timing_budget(d);

    _wr(d, VL53_SYSTEM_SEQUENCE_CONFIG, 0xE8);
    _set_timing_budget(d, 50000);

    /* Calibração VHV */
    _wr(d, VL53_SYSTEM_SEQUENCE_CONFIG, 0x01);
    if (!_single_ref_cal(d, 0x40)) return false;

    /* Calibração de fase */
    _wr(d, VL53_SYSTEM_SEQUENCE_CONFIG, 0x02);
    if (!_single_ref_cal(d, 0x00)) return false;

    /* Restaura sequência */
    _wr(d, VL53_SYSTEM_SEQUENCE_CONFIG, 0xE8);

    LOG("[VL53] Sensor 0x%02X OK.\n", d->addr);
    return true;
}

/* --------------------------------------------------------------------------
 * API pública
 * -------------------------------------------------------------------------- */

bool vl53l0x_init(vl53l0x_t *dev, i2c_inst_t *i2c, uint8_t addr, uint16_t timeout_ms)
{
    dev->i2c              = i2c;
    dev->addr             = addr;
    dev->io_timeout       = timeout_ms;
    dev->did_timeout      = false;
    dev->stop_variable    = 0;
    dev->timing_budget_us = 0;
    return _do_init(dev);
}

bool vl53l0x_init_dual(vl53l0x_t *dev_a, vl53l0x_t *dev_b,
                        i2c_inst_t *i2c,
                        uint8_t xshut_a, uint8_t xshut_b,
                        uint16_t timeout_ms)
{
    gpio_init(xshut_a); gpio_set_dir(xshut_a, GPIO_OUT);
    gpio_init(xshut_b); gpio_set_dir(xshut_b, GPIO_OUT);
    gpio_put(xshut_a, 0); gpio_put(xshut_b, 0);
    sleep_ms(10);

    /* Sensor B sobe SOZINHO em 0x29 (A ainda está em XSHUT LOW) e é reprogramado
     * para 0x30 antes de A subir — evita que o write 0x29 atinja os dois sensores */
    gpio_put(xshut_b, 1); sleep_ms(10);
    LOG("[VL53] Reprogramando sensor B para 0x30...\n");
    uint8_t cmd[2] = {VL53_I2C_SLAVE_DEVICE_ADDRESS, VL53L0X_ADDR_B};
    i2c_write_blocking(i2c, 0x29, cmd, 2, false);
    sleep_ms(5);
    LOG("[VL53] Inicializando sensor B (0x30)...\n");
    if (!vl53l0x_init(dev_b, i2c, VL53L0X_ADDR_B, timeout_ms)) return false;

    /* Sensor A sobe SOZINHO em 0x29 (B já está em 0x30 — sem conflito) */
    gpio_put(xshut_a, 1); sleep_ms(10);
    LOG("[VL53] Inicializando sensor A (0x29)...\n");
    if (!vl53l0x_init(dev_a, i2c, VL53L0X_ADDR_A, timeout_ms)) return false;

    return true;
}

uint16_t vl53l0x_read_single_mm(vl53l0x_t *dev)
{
    /* Arma */
    _wr(dev, 0x80, 0x01); _wr(dev, 0xFF, 0x01); _wr(dev, 0x00, 0x00);
    _wr(dev, 0x91, dev->stop_variable);
    _wr(dev, 0x00, 0x01); _wr(dev, 0xFF, 0x00); _wr(dev, 0x80, 0x00);

    /* Dispara */
    _wr(dev, VL53_SYSRANGE_START, 0x01);

    /* Aguarda start bit ser limpo pelo sensor */
    _timeout_start(dev);
    while (_rd(dev, VL53_SYSRANGE_START) & 0x01) {
        if (_timeout_expired(dev)) {
            dev->did_timeout = true;
            LOG("[VL53] 0x%02X timeout (start bit)\n", dev->addr);
            return VL53L0X_OUT_OF_RANGE;
        }
    }

    /* Aguarda dado pronto */
    _timeout_start(dev);
    while ((_rd(dev, VL53_RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if (_timeout_expired(dev)) {
            dev->did_timeout = true;
            LOG("[VL53] 0x%02X timeout (interrupt)\n", dev->addr);
            return VL53L0X_OUT_OF_RANGE;
        }
    }

    /* Lê resultado — RESULT_RANGE_STATUS + 10 */
    uint16_t mm = _rd16(dev, VL53_RESULT_RANGE_STATUS + 10);

    /* Limpa flag */
    _wr(dev, VL53_SYSTEM_INTERRUPT_CLEAR, 0x01);

    return mm;
}
