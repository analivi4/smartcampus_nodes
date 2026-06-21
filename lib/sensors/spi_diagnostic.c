/*
 * spi_diagnostic.c — Diagnóstico SPI/SX1276
 * UFC Quixadá Smart Campus
 *
 * Implementação fiel ao spi_diagnostic.c do Diogoassun/Pico_LoRaWAN,
 * refatorada como biblioteca reutilizável pelos três nós.
 */

#include "spi_diagnostic.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdio.h>

/* Registradores do SX1276 */
#define REG_VERSION  0x42   /* Deve retornar 0x12 */
#define REG_OPMODE   0x01
#define REG_PACONFIG 0x09

/* --------------------------------------------------------------------------
 * Helpers internos de SPI raw (sem a stack LoRaWAN)
 * -------------------------------------------------------------------------- */

static spi_inst_t *_spi;
static uint8_t    _nss;

static void _select(void)   { gpio_put(_nss, 0); }
static void _deselect(void) { gpio_put(_nss, 1); }

static uint8_t _read_reg(uint8_t reg)
{
    uint8_t addr = reg & 0x7F;
    uint8_t val  = 0;
    _select();
    spi_write_blocking(_spi, &addr, 1);
    spi_read_blocking(_spi, 0x00, &val, 1);
    _deselect();
    return val;
}

static void _write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { (uint8_t)(reg | 0x80), val };
    _select();
    spi_write_blocking(_spi, buf, 2);
    _deselect();
}

/* --------------------------------------------------------------------------
 * API pública
 * -------------------------------------------------------------------------- */

void spi_diagnostic_init(const spi_diag_config_t *cfg)
{
    _spi = cfg->spi_inst;
    _nss = cfg->nss;

    gpio_init(cfg->nss);
    gpio_set_dir(cfg->nss, GPIO_OUT);
    gpio_put(cfg->nss, 1);
    sleep_ms(5);

    spi_init(cfg->spi_inst, 1 * 1000 * 1000);
    gpio_set_function(cfg->sck,  GPIO_FUNC_SPI);
    gpio_set_function(cfg->mosi, GPIO_FUNC_SPI);
    gpio_set_function(cfg->miso, GPIO_FUNC_SPI);
    sleep_ms(5);
}

bool spi_diagnostic_run(const spi_diag_config_t *cfg)
{
    _spi = cfg->spi_inst;
    _nss = cfg->nss;

    LOG("\n========================================\n");
    LOG("  SX1276 SPI DIAGNOSTIC\n");
    LOG("  (Diogoassun/Pico_LoRaWAN pattern)\n");
    LOG("========================================\n");

    LOG("[1] Reset do radio...\n");
    gpio_init(cfg->reset);
    gpio_set_dir(cfg->reset, GPIO_OUT);
    gpio_put(cfg->reset, 0);
    sleep_ms(10);
    gpio_set_dir(cfg->reset, GPIO_IN);
    gpio_pull_up(cfg->reset);
    sleep_ms(20);
    LOG("    Reset concluido.\n");

    LOG("[2] Lendo VERSION (0x42) — esperado 0x12...\n");
    uint8_t version = 0;
    for (int t = 0; t < 3; t++) {
        version = _read_reg(REG_VERSION);
        LOG("    Tentativa %d: 0x%02X\n", t + 1, version);
        sleep_ms(50);
    }

    if (version == 0x12) {
        LOG("    [OK] Radio detectado!\n");
    } else if (version == 0x00) {
        LOG("    [ERRO] 0x00 — MISO preso em GND ou NSS nao funciona.\n");
        LOG("           Verifique solda do MISO (GPIO%d) e NSS (GPIO%d).\n",
               cfg->miso, cfg->nss);
    } else if (version == 0xFF) {
        LOG("    [ERRO] 0xFF — MISO preso em VCC ou MOSI/MISO invertidos.\n");
    } else {
        LOG("    [AVISO] 0x%02X inesperado — ruido ou modulo diferente.\n", version);
    }

    LOG("[3] Teste escrita/leitura (RegOpMode)...\n");
    _write_reg(REG_OPMODE, 0x00);
    sleep_ms(10);
    uint8_t opmode = _read_reg(REG_OPMODE);
    LOG("    Escreveu 0x00, leu 0x%02X — %s\n",
           opmode, opmode == 0x00 ? "[OK]" : "[FALHA] Verifique MOSI.");

    _write_reg(REG_OPMODE, 0x80);
    sleep_ms(10);
    opmode = _read_reg(REG_OPMODE);
    LOG("    Escreveu 0x80, leu 0x%02X — %s\n",
           opmode, opmode == 0x80 ? "[OK] Modo LoRa ativado." : "[FALHA]");

    gpio_init(cfg->dio0);
    gpio_set_dir(cfg->dio0, GPIO_IN);
    LOG("[4] DIO0 (GPIO%d) em repouso: %d (esperado: 0)\n",
           cfg->dio0, gpio_get(cfg->dio0));

    LOG("[5] Repetindo leitura a 8MHz...\n");
    spi_set_baudrate(cfg->spi_inst, 8 * 1000 * 1000);
    sleep_ms(5);
    uint8_t version_8mhz = _read_reg(REG_VERSION);
    LOG("    VERSION a 8MHz: 0x%02X — %s\n",
           version_8mhz,
           version_8mhz == 0x12 ? "[OK]" : "[FALHA] Use 1MHz.");

    spi_set_baudrate(cfg->spi_inst, 1 * 1000 * 1000);

    LOG("========================================\n");
    if (version == 0x12) {
        LOG("  RESULTADO: RADIO OK\n");
        LOG("  Pode prosseguir com a stack LoRaWAN.\n");
    } else {
        LOG("  RESULTADO: FALHA DE COMUNICACAO SPI\n");
        LOG("  Checklist:\n");
        LOG("  [ ] SCK/MOSI/MISO/NSS/RESET conectados?\n");
        LOG("  [ ] Modulo em 3.3V (nao 5V)?\n");
        LOG("  [ ] Soldas firmes?\n");
        LOG("  [ ] MISO com pull-up externo?\n");
    }
    LOG("========================================\n\n");

    return (version == 0x12);
}
