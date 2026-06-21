#include <stdio.h>
#include "pico/stdlib.h"
#include "ir_raw.h"
#include "ir_ac_commands.h"

#define IR_TX_PIN 3

int main(void) {
    stdio_init_all(); sleep_ms(2000);
    printf("=== TESTE CAMADA 4: IR raw ===\n");
    ir_raw_init(IR_TX_PIN);

    int cycle = 0;
    while(1) {
       // if (cycle % 2 == 0) {
            printf("Enviando IR_CMD_ON...\n");
          ir_raw_send(IR_TX_PIN, IR_CMD_ON, IR_CMD_LENGTH);
       // } else {
         //   printf("Enviando IR_CMD_OFF...\n");
         //   ir_raw_send(IR_TX_PIN, IR_CMD_OFF, IR_CMD_LENGTH);
       // }
       // cycle++;

        /* Com câmera frontal do celular: LED deve piscar rapidamente durante ~30ms */
        sleep_ms(3000);
    }
}