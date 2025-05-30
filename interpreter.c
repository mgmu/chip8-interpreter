#include "interpreter.h"
#include <stddef.h>
#include <stdio.h>

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

void init(struct interpreter *chip) {
    if (chip == NULL)
        return;
    for (int i = 0; i < RAM_SIZE; i++)
        chip->ram[i] = (uint8_t)0;
    for (int i = 0; i < REGISTERS_SIZE; i++)
        chip->registers[i] = (uint8_t)0;
    chip->I  = (uint8_t)0;
    chip->pc = (uint16_t)PC_INIT;
    chip->sp = (uint8_t)0;
    chip->dt = (uint8_t)0;
    chip->st = (uint8_t)0;
    for (int i = 0; i < LEVELS_SIZE; i++)
        chip->stack[i] = (uint16_t)0;
    for (int i = 0; i < VBUF_HEIGHT; i++) {
        for (int j = 0; j < VBUF_WIDTH; j++)
            chip->vbuf[i * VBUF_WIDTH + j] = (uint32_t)PIXEL_OFF;
    }
    for (int i = 0; i < CHAR_SPRITES_SIZE; i++)
        chip->ram[CHAR_SPRITES_ADDR + i] = char_sprites[i];
    for (int i = 0; i < KEYBOARD_SIZE; i++)
        chip->keyboard[i] = (uint8_t)KEY_UP;
    for (int i = 0; i < KEYBOARD_SIZE; i++)
        chip->prev_keyboard[i] = (uint8_t)KEY_UP;
    chip->checking_key_press = 0;
    chip->update_display = 1;
    srandom(time(NULL));
}

int dec_exec0(uint16_t instr, struct interpreter *chip) {
    if (chip == NULL)
        return -1;
    switch (instr) {
      case 0x00e0:
        chip->update_display = 1;
        for (int i = 0; i < VBUF_HEIGHT; i++) {
            for (int j = 0; j < VBUF_WIDTH; j++)
                chip->vbuf[i * VBUF_WIDTH + j] = 0;
        }
        break;
      case 0x00ee:
        chip->pc = chip->stack[chip->sp];
        chip->sp--;
        break;
      default:
        // ignore 0x0nnn
        break;
    }
    return 0;
}

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
        chip->registers[x] = chip->registers[x] - chip->registers[y];
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
      case 14:
        if ((chip->registers[x] & 128) == 1)
            chip->registers[VF] = 1;
        else
            chip->registers[VF] = 0;
        chip->registers[x] *= 2;
        break;
      default:
        return -1;
    }
    return 0;
}

int dec_execE(uint16_t x, uint16_t kk, struct interpreter *chip) {
    switch (kk) {
      case 0x9e:
        if (chip->keyboard[x] == KEY_DOWN)
            chip->pc += 2;
        break;
      case 0xa1:
        if (chip->keyboard[x] == KEY_UP)
            chip->pc += 2;
        break;
      default:
        return -1;
    }
    return 0;
}

int dec_execF(uint16_t x, uint16_t kk, struct interpreter *chip) {
    uint8_t tmp = 0;
    uint8_t key_pressed = 0;
    switch (kk) {
      case 0x07:
        chip->registers[x] = chip->dt;
        break;
      case 0x0A:
        if (!chip->checking_key_press) {
            for (int i = 0; i < KEYBOARD_SIZE; i++) {
                if (chip->keyboard[i] == KEY_DOWN)
                    chip->prev_keyboard[i] == KEY_DOWN;
                else
                    chip->prev_keyboard[i] == KEY_UP;
            }
            chip->checking_key_press = 1;
            chip->pc -= 2;
        } else {
            for (int i = 0; i < KEYBOARD_SIZE; i++) {
                if (chip->keyboard[i] == KEY_UP
                        && chip->prev_keyboard[i] == KEY_DOWN) {
                    chip->checking_key_press = 0;
                    chip->registers[x] = i;
                    key_pressed = 1;
                    break;
                }
            }
            if (!key_pressed)
                chip->pc -= 2;
            for (int i = 0; i < KEYBOARD_SIZE; i++)
                chip->prev_keyboard[i] = chip->keyboard[i];
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
        for (int i = 0; i <= x; i++)
            chip->ram[chip->I + i] = chip->registers[i];
        break;
      case 0x65:
        for (int i = 0; i <= x; i++)
            chip->registers[i] = chip->ram[chip->I + i];
        break;
      default:
        return -1;
    }
    return 0;
}

int dec_exec(const uint16_t instr, struct interpreter *chip, int mode) {
    if (chip == NULL)
        return -1;
    int      res    = 0;
    uint16_t opcode = instr >> 12;
    uint16_t nnn    = instr & NNN_MASK;
    uint8_t n       = instr & N_MASK;
    uint8_t x       = (instr & X_MASK) >> 8;
    uint8_t y       = (instr & Y_MASK) >> 4;
    uint8_t kk      = instr & KK_MASK;
    if (mode) {
        printf("\nCYCLE:\n");
        printf("instr:  %#06x n:  %#06x\n", instr, n);
        printf("opcode: %#06x x:  %#06x\n", opcode, x);
        printf("nnn:    %#06x y:  %#06x\n", nnn, y);
        printf("kk:     %#06x pc: %#05x\n", kk, chip->pc);
        int c = 0;
        for (int i = 0; i < 15; i++) {
            printf("reg[%02d]: %03d ", i, chip->registers[i]);
            if (c++ % 5 == 4)
                printf("\n");
        }
        printf("checking key press: %d\n", chip->checking_key_press);
    }
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
        chip->registers[x] = kk & (random() % 256);
        break;
      case 0xd: // Dxyn
        chip->registers[VF] = 0;
        for (uint8_t i = 0; i < n; i++) {
            uint8_t byte = chip->ram[chip->I + i];
            for (uint8_t j = 0; j < 8; j++) {
                uint8_t bit  = (byte >> (7-j)) & 1;
                int     line = (chip->registers[y] + i) % VBUF_HEIGHT;
                int     col  = (chip->registers[x] + j) % VBUF_WIDTH;
                if (chip->vbuf[line * VBUF_WIDTH + col] == PIXEL_ON && bit == 1)
                    chip->registers[VF] = 1;
                uint32_t new_pixel = 0;
                if (bit == 1)
                    new_pixel = PIXEL_ON;
                chip->vbuf[line * VBUF_WIDTH + col] ^= new_pixel;
            }
        }
        chip->update_display = 1;
        break;
      case 0xe: // Ex9E, ExA1
        res = dec_execE(x, kk, chip);
        break;
      case 0xf: // Fx07, Fx0A, Fx15, Fx18, Fx1C, Fx29, Fx33, Fx55, Fx65
        res = dec_execF(x, kk, chip);
        break;
      default:
        res = -1;
        break;
    }
    return res;
}

void run_rom_cycle(struct interpreter *chip, struct proc_state *ps, int mode) {
    // check that pc is still in program
    if (chip->pc > RAM_SIZE) {
        ps->curr_instr = 0;
        ps->pc = chip->pc;
        ps->err_code = 1;
        return;
    }

    // read instruction
    uint16_t lb = (uint16_t)chip->ram[chip->pc] << 8;
    uint16_t rb = (uint16_t)chip->ram[chip->pc + 1];
    uint16_t instr = lb | rb;
    ps->curr_instr = instr;

    // set program counter to next instruction
    chip->pc += 2;
    ps->pc = chip->pc;
    if (instr == 0)
        return;

    // decode and execute instruction
    int res = dec_exec(instr, chip, mode);
    if (res < 0) {
        ps->err_code = 1;
        return;
    }

    // update timers
    if (chip->dt > 0)
        chip->dt--;
    if (chip->st > 0)
        chip->st--;
}
