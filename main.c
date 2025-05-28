#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <time.h>

#define RAM_SIZE          4096       // The size in bytes of the ram
#define PC_INIT           0x0200     // The initial address that PC points to
#define REGISTERS_SIZE    16         // The size of the registers array
#define VF                15         // The VF register
#define LEVELS_SIZE       16         // The size of the execution stack
#define CHAR_SPRITES_SIZE 80         // The size in bytes of all char sprites
#define CHAR_SPRITE_SIZE  5          // The size in bytes of a char sprite
#define CHAR_SPRITES_ADDR 0x0050     // The address of the first char sprite
#define KEYBOARD_SIZE     16         // The number of keys of the keyboard
#define KEY_UP            0          // The value of a key if it is up
#define KEY_DOWN          255        // The value of a key if is down (pressed)
#define MAX_ROM_SIZE      3840       // The maximal size in bytes of a ROM
#define VBUF_WIDTH        64         // The width of the video buffer
#define VBUF_HEIGHT       32         // The height of the video buffer
#define PIXEL_ON          0xffffffff // The value of a pixel on
#define PIXEL_OFF         0          // The value of a pixel off
#define NNN_MASK          0x0fff     // Mask of nnn/addr value in an instruction
#define N_MASK            0x000f     // Mask of n/nibble value in an instruction
#define X_MASK            0x0f00     // Mask of x value in an instruction
#define Y_MASK            0x00f0     // Mask of y value in an instruction
#define KK_MASK           0x00ff     // Mask of kk/byte value in an instruction

uint8_t char_sprites[CHAR_SPRITES_SIZE] = {
    0xf0, 0x90, 0x90, 0x90, 0xf0, // "0"
    0x20, 0x60, 0x20, 0x20, 0x70, // "1"
    0xf0, 0x10, 0xf0, 0x80, 0xf0, // "2"
    0xf0, 0x10, 0xf0, 0x10, 0xf0, // "3"
    0x90, 0x90, 0xf0, 0x10, 0x10, // "4"
    0xf0, 0x80, 0xf0, 0x10, 0xf0, // "5"
    0xf0, 0x80, 0xf0, 0x90, 0xf0, // "6"
    0xf0, 0x10, 0x20, 0x40, 0x40, // "7"
    0xf0, 0x90, 0xf0, 0x90, 0xf0, // "8"
    0xf0, 0x90, 0xf0, 0x10, 0xf0, // "9"
    0xf0, 0x90, 0xf0, 0x90, 0x90, // "A"
    0xe0, 0x90, 0xe0, 0x90, 0xe0, // "B"
    0xf0, 0x80, 0x80, 0x80, 0xf0, // "C"
    0xe0, 0x90, 0x90, 0x90, 0xe0, // "D"
    0xf0, 0x80, 0xf0, 0x80, 0xf0, // "E"
    0xf0, 0x80, 0xf0, 0x80, 0x80  // "F"
};

struct interpreter {
    uint8_t  ram[RAM_SIZE];                  // 4Kb memory space
    uint8_t  registers[REGISTERS_SIZE];      // general purpose registers
    uint16_t I;                              // addr storage register
    uint16_t pc;                             // program counter
    uint8_t  sp;                             // stack pointer
    uint8_t  dt;                             // delay timer
    uint8_t  st;                             // sound timer
    uint16_t stack[LEVELS_SIZE];             // execution stack
    uint32_t vbuf[VBUF_HEIGHT][VBUF_WIDTH];  // video buffer
    uint8_t  keyboard[KEYBOARD_SIZE];        // keyboard
};

/*
 * Initializes chip memory.
 * All registers and ram are zero'ed, PC is set to its initial value and
 * characters sprites are loaded at 0x0050 (up to 0x00A0, 80 bytes in total).
 * A seed is also provided to the random number generator.
 */
void init(struct interpreter *chip) {
    if (chip == NULL)
        return;
    bzero(chip->ram, RAM_SIZE);
    bzero(chip->registers, REGISTERS_SIZE);
    chip->I  = 0;
    chip->pc = (uint16_t)PC_INIT;
    chip->sp = 0;
    chip->dt = 0;
    chip->st = 0;
    bzero(chip->stack, LEVELS_SIZE);
    bzero(chip->vbuf, VBUF_WIDTH * VBUF_HEIGHT);
    for (int i = 0; i < CHAR_SPRITES_SIZE; i++)
        chip->ram[CHAR_SPRITES_ADDR + i] = char_sprites[i];
    bzero(chip->keyboard, KEYBOARD_SIZE);
    srandom(time(NULL));
}

/*
 * Decodes and executes 0nnn, 00E0, 00EE instructions. 0nnn is in fact ignored.
 */
int dec_exec0(uint16_t instr, struct interpreter *chip) {
    if (chip == NULL)
        return -1;
    switch (instr) {
      case 0x00e0:
        bzero(chip->vbuf, VBUF_WIDTH * VBUF_HEIGHT);
        break;
      case 0x00ee:
        chip->pc = chip->stack[chip->sp];
        chip->sp--;
        break;
      default:
        return -1;
    }
    return 0;
}

/*
 * Decodes and executes the instructions 8xy0, ..., 8xy7, 8xyE.
 */
int dec_exec8(uint16_t n, uint16_t x, uint16_t y, struct interpreter *chip) {
    if (chip == NULL)
        return -1;
    switch (n) {
      case 0:
        chip->registers[x] = chip->registers[y];
        break;
      case 1:
        chip->registers[x] |= chip->registers[y];
        break;
      case 2:
        chip->registers[x] &= chip->registers[y];
        break;
      case 3:
        chip->registers[x] ^= chip->registers[y];
        break;
      case 4:
        chip->registers[x] += chip->registers[y];
        if (chip->registers[x] < chip->registers[y]) // bigger than 8 bits
            chip->registers[VF] = 1;
        else
            chip->registers[VF] = 0;
        break;
      case 5:
        if (chip->registers[x] > chip->registers[y])
            chip->registers[VF] = 1;
        else
            chip->registers[VF] = 0;
        break;
      case 6:
        if ((chip->registers[x] & 1) == 1)
            chip->registers[VF] = 1;
        else
            chip->registers[VF] = 0;
        chip->registers[x] /= 2;
        break;
      case 7:
        if (chip->registers[y] > chip->registers[x])
            chip->registers[VF] = 1;
        else
            chip->registers[VF] = 0;
        chip->registers[x] = chip->registers[y] - chip->registers[x];
        break;
      case 15:
        if ((chip->registers[x] & 128) == 1)
            chip->registers[VF] = 1;
        else
            chip->registers[VF] = 0;
        chip->registers[x] *= 2;
      default:
        return -1;
    }
    return 0;
}

/*
 * Decodes and executes the instructions Ex9E, ExA1.
 */
int dec_execE(uint16_t x, uint16_t kk, struct interpreter *chip) {
    switch (kk) {
      case 0x9e:
        if (chip->keyboard[x] == KEY_DOWN)
            chip->pc += 2;
        break;
      case 0xa1:
        if (chip->keyboard[x] == KEY_UP)
            chip->pc += 2;
      default:
        return -1;
    }
    return 0;
}

/*
 * Decodes and executes the instructions Fx07, Fx0A, Fx15, Fx18, Fx1C, Fx29,
 * Fx33, Fx55, Fx65.
 */
int dec_execF(uint16_t x, uint16_t kk, struct interpreter *chip) {
    uint8_t tmp = 0;
    int no_down = 1;
    switch (kk) {
      case 0x07:
        chip->registers[x] = chip->dt;
        break;
      case 0x0A:
        while (no_down) {
            for (int i = 0; i < KEYBOARD_SIZE; i++) {
                if (chip->keyboard[i] == KEY_DOWN) {
                    no_down = 0;
                    chip->registers[x] = i;
                    break;
                }
            }
        }
        break;
      case 0x15:
        chip->dt = chip->registers[x];
        break;
      case 0x18:
        chip->st = chip->registers[x];
        break;
      case 0x1e:
        chip->I += chip->registers[x];
        break;
      case 0x29:
        chip->I = CHAR_SPRITES_ADDR + CHAR_SPRITE_SIZE * chip->registers[x];
        break;
      case 0x33:
        tmp = chip->registers[x];
        chip->ram[chip->I] = tmp / 100;
        tmp -= chip->ram[chip->I] * 100;
        chip->ram[chip->I + 1] = tmp / 10;
        tmp -= chip->ram[chip->I + 1] * 10;
        chip->ram[chip->I + 2] = tmp;
        break;
      case 0x55:
        for (int i = 0; i < x; i++)
            chip->ram[chip->I + i] = chip->registers[x];
        break;
      case 0x65:
        for (int i = 0; i < x; i++)
            chip->registers[i] = chip->ram[chip->I + i];
        break;
      default:
        return -1;
    }
    return 0;
}

/*
 * Decodes the given instruction and executes it. On success, returns 0, -1
 * otherwise.
 */
int dec_exec(const uint16_t instr, struct interpreter *chip) {
    if (chip == NULL)
        return -1;
    int      res    = 0;
    uint16_t opcode = instr >> 12;
    uint16_t nnn    = instr & NNN_MASK;
    uint8_t n       = instr & N_MASK;
    uint8_t x       = (instr & X_MASK) >> 8;
    uint8_t y       = (instr & Y_MASK) >> 8;
    uint8_t kk      = instr & KK_MASK;
    switch (opcode) {
      case 0x0: // 0nnn, 00e0, 00ee
        res = dec_exec0(instr, chip);
        break;
      case 0x1: // 1nnn
        chip->pc = nnn;
        break;
      case 0x2: // 2nnn
        chip->sp++;
        chip->stack[chip->sp] = chip->pc;
        chip->pc              = nnn;
        break;
      case 0x3: // 3xkk
        if (chip->registers[x] == kk)
            chip->pc += 2;
        break;
      case 0x4: // 4xkk
        if (chip->registers[x] != kk)
            chip->pc += 2;
        break;
      case 0x5: // 5xy0
        if (n != 0) {
            res = -1;
            break;
        }
        if (chip->registers[x] == chip->registers[y])
            chip->pc += 2;
        break;
      case 0x6: // 6xkk
        chip->registers[x] = kk;
        break;
      case 0x7: // 7xkk
        chip->registers[x] += kk;
        break;
      case 0x8: // 8xy0, ..., 8xy7, 8xyE
        res = dec_exec8(n, x, y, chip);
        break;
      case 0x9: // 9xy0
        if (n != 0) {
            res = -1;
            break;
        }
        if (chip->registers[x] != chip->registers[y])
            chip->pc += 2;
        break;
      case 0xa: // Annn
        chip->I = nnn;
        break;
      case 0xb: // Bnnn
        chip->pc = nnn + chip->registers[0];
        break;
      case 0xc: // Cxkk
        chip->registers[x] == kk & (random() % 256);
        break;
      case 0xd: // Dxyn
        chip->registers[VF] = 0;
        uint8_t vx = chip->registers[x];
        uint8_t vy = chip->registers[y];
        for (uint8_t i = 0; i < n; i++) {
            uint8_t byte = chip->ram[chip->I + i];
            for (uint8_t j = 0; j < 8; j++) {
                uint8_t bit  = (byte >> j) & 1;
                int     line = (chip->registers[x] + i) % VBUF_WIDTH;
                int     col  = (chip->registers[y] + j) % VBUF_HEIGHT;
                if (chip->vbuf[line][col] == PIXEL_ON && bit == 2)
                    chip->registers[VF] = 1;
                chip->vbuf[line][col] ^= bit;
            }
        }
        break;
      case 0xe: // Ex9E, ExA1
        res = dec_execE(x, kk, chip);
        break;
      case 0xf: // Fx07, Fx0A, Fx15, Fx18, Fx1C, Fx29, Fx33, Fx55, Fx65
        res = dec_execF(x, kk, chip);
      default:
        res = -1;
        break;
    }
    return res;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, "Missing file name\n");
        return EXIT_FAILURE;
    }

    struct stat sb;
    if (stat(argv[1], &sb) == -1) {
        perror("stat()");
        return EXIT_FAILURE;
    }

    if (!S_ISREG(sb.st_mode)) {
        dprintf(STDERR_FILENO, "Not a regular file\n");
        return EXIT_FAILURE;
    }

    if (sb.st_size > MAX_ROM_SIZE) {
        dprintf(STDERR_FILENO, "File too large\n");
        return EXIT_FAILURE;
    }

    if (sb.st_size <= 0) {
        dprintf(STDERR_FILENO, "File too short\n");
        return EXIT_FAILURE;
    }

    if (sb.st_size % 2 != 0) { // each instruction is 2 bytes long
        dprintf(STDERR_FILENO, "File is corrupted\n");
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open()");
        return EXIT_FAILURE;
    }

    struct interpreter chip;
    init(&chip);

    if (read(fd, chip.ram + chip.pc, sb.st_size) < 0) {
        perror("read()");
        close(fd);
        return EXIT_FAILURE;
    }

    if (close(fd) < 0) {
        perror("close()");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
