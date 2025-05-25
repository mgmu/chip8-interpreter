#include <stdint.h>
#define RAM_SIZE     4096
#define PC_INIT      256        // addr 0x0200
#define NB_REGISTERS 16
#define NB_LEVELS    16

struct interpreter {
    uint8_t  ram[RAM_SIZE];     // 4Kb memory space
    uint8_t  registers[NB_REGISTERS]; // general purpose registers
    uint16_t idx;               // addr storage register
    uint8_t  pc;                // program counter
    uint8_t  sp;                // stack pointer
    uint16_t stack[NB_LEVELS];  // execution stack
};
typedef struct interpreter chip;

int main() {
    return 0;
}
