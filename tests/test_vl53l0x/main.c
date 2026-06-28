#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "vl53l0x.h"

#define SDA 4
#define SCL 5
#define XSHUT_A 7
#define XSHUT_B 8

int main(void)
{
    stdio_init_all(); sleep_ms(2000);
    printf("=== TESTE CAMADA 3B: VL53L0X ===\n");

    i2c_init(i2c0, 400000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SDA); gpio_pull_up(SCL);

    vl53l0x_t sa, sb;
    bool ok = vl53l0x_init_dual(&sa, &sb, i2c0, XSHUT_A, XSHUT_B, 500);
    printf("Init dual: %s\n\n", ok ? "PASS" : "FAIL");
    if (!ok) while(1);

    while (1) {
        uint16_t da = vl53l0x_read_single_mm(&sa);
        uint16_t db = vl53l0x_read_single_mm(&sb);
        printf("A: %4dmm  |  B: %4dmm", da, db);
        if (da < 500) printf("  << A detectou!");
        if (db < 500) printf("  << B detectou!");
        printf("\n");
        sleep_ms(200);
    }
}