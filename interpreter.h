#ifndef INTERPRETER_H
#define INTERPRETER_H
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <strings.h>

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
#define VBUF_WIDTH        64         // The width of the video buffer
#define VBUF_HEIGHT       32         // The height of the video buffer
#define PIXEL_ON          0xffffffff // The value of a pixel on
#define PIXEL_OFF         0          // The value of a pixel off
#define NNN_MASK          0x0fff     // Mask of nnn/addr value in an instruction
#define N_MASK            0x000f     // Mask of n/nibble value in an instruction
#define X_MASK            0x0f00     // Mask of x value in an instruction
#define Y_MASK            0x00f0     // Mask of y value in an instruction
#define KK_MASK           0x00ff     // Mask of kk/byte value in an instruction

struct interpreter {
    uint8_t  ram[RAM_SIZE];                  // 4Kb memory space
    uint8_t  registers[REGISTERS_SIZE];      // general purpose registers
    uint16_t I;                              // addr storage register
    uint16_t pc;                             // program counter
    uint8_t  sp;                             // stack pointer
    uint8_t  dt;                             // delay timer
    uint8_t  st;                             // sound timer
    uint16_t stack[LEVELS_SIZE];             // execution stack
    uint32_t vbuf[VBUF_HEIGHT * VBUF_WIDTH]; // video buffer
    uint8_t  keyboard[KEYBOARD_SIZE];        // current state of keyboard
    uint8_t  prev_keyboard[KEYBOARD_SIZE];   // previous state of keyboard
    uint8_t  checking_key_press;             // flag for key press check
    uint8_t  update_display;                 // update display flag
};

struct proc_state {
    uint16_t curr_instr;        // current instruction
    uint16_t pc;                // value of interpreter program counter
    uint8_t  err_code;          // error code
};

/*
 * Initializes chip8 interpreter memory.
 * All registers and ram are zero'ed, PC is set to its initial value and
 * characters sprites are loaded at 0x0050 (up to 0x00A0, 80 bytes in total).
 * A seed is also provided to the random number generator.
 */
void init(struct interpreter *chip);

/*
 * Decodes and executes 0nnn, 00E0, 00EE instructions. 0nnn is in fact ignored.
 */
int dec_exec0(uint16_t instr, struct interpreter *chip);

/*
 * Decodes and executes the instructions 8xy0, ..., 8xy7, 8xyE.
 */
int dec_exec8(uint16_t n, uint16_t x, uint16_t y, struct interpreter *chip);

/*
 * Decodes and executes the instructions Ex9E, ExA1.
 */
int dec_execE(uint16_t x, uint16_t kk, struct interpreter *chip);

/*
 * Decodes and executes the instructions Fx07, Fx0A, Fx15, Fx18, Fx1C, Fx29,
 * Fx33, Fx55, Fx65.
 */
int dec_execF(uint16_t x, uint16_t kk, struct interpreter *chip);

/*
 * Decodes the given instruction and executes it. On success, returns 0, -1
 * otherwise. If mode is debug (mode != 0), prints to stdout what the
 * decoder/executer is doing.
 */
int dec_exec(const uint16_t instr, struct interpreter *chip, int mode);

/*
 * Runs one cycle of the ROM loaded in chip. Populates ps with appropriate
 * values about the cycle termination state. Chip and ps must be previously
 * initialized. If mode is 0, runs the cycle normally, otherwise runs it in
 * debug mode (prints to standard output information about the cycle).
 */
void run_rom_cycle(struct interpreter *chip, struct proc_state *ps, int mode);

#endif
