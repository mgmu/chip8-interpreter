#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include "interpreter.h"

#define MISSING_FILE_ERR "Missing file name\n"
#define NOT_REGULAR_ERR  "Not a regular file\n"
#define TOO_LARGE_ERR    "File too large\n"
#define TOO_SHORT_ERR    "File too short\n"
#define CORRUPTED_ERR    "File is corrupted\n"
#define MAX_ROM_SIZE     3840   // The maximal size in bytes of a ROM

int main(int argc, char **argv) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, MISSING_FILE_ERR);
        return EXIT_FAILURE;
    }

    struct stat sb;
    if (stat(argv[1], &sb) == -1) {
        perror("stat()");
        return EXIT_FAILURE;
    }

    if (!S_ISREG(sb.st_mode)) {
        dprintf(STDERR_FILENO, NOT_REGULAR_ERR);
        return EXIT_FAILURE;
    }

    if (sb.st_size > MAX_ROM_SIZE) {
        dprintf(STDERR_FILENO, TOO_LARGE_ERR);
        return EXIT_FAILURE;
    }

    if (sb.st_size <= 0) {
        dprintf(STDERR_FILENO, TOO_SHORT_ERR);
        return EXIT_FAILURE;
    }

    if (sb.st_size % 2 != 0) { // each instruction is 2 bytes long
        dprintf(STDERR_FILENO, CORRUPTED_ERR);
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
