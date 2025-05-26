#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#define RAM_SIZE       4096
#define PC_INIT        256      // addr 0x0200
#define REGISTERS_SIZE 16
#define LEVELS_SIZE    16
#define MAX_ROM_LEN    3840

struct interpreter {
    uint8_t  ram[RAM_SIZE];     // 4Kb memory space
    uint8_t  registers[REGISTERS_SIZE]; // general purpose registers
    uint16_t idx;               // addr storage register
    uint16_t pc;                // program counter
    uint8_t  sp;                // stack pointer
    uint16_t stack[LEVELS_SIZE]; // execution stack
};

void init(struct interpreter *chip) {
    if (chip == NULL)
        return;
    bzero(chip->ram, RAM_SIZE);
    bzero(chip->registers, REGISTERS_SIZE);
    chip->idx = 0;
    chip->pc  = (uint16_t)PC_INIT;
    chip->sp  = 0;
    bzero(chip->stack, LEVELS_SIZE);
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

    if (sb.st_size > MAX_ROM_LEN) {
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
