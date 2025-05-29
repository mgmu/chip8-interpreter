#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "interpreter.h"

#define MISSING_FILE_ERR "Missing file name\n"
#define NOT_REGULAR_ERR  "Not a regular file\n"
#define TOO_LARGE_ERR    "File too large\n"
#define TOO_SHORT_ERR    "File too short\n"
#define CORRUPTED_ERR    "File is corrupted\n"
#define MAX_ROM_SIZE     3840   // The maximal size in bytes of a ROM

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
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

    // Initializing GUI
    SDL_Window      *window;
    SDL_Renderer    *renderer;
    int              scale  = atoi(argv[2]);
    int              width  = VBUF_WIDTH * scale;
    int              height = VBUF_HEIGHT * scale;
    bool             done   = false;
    SDL_WindowFlags  flags  = SDL_WINDOW_OPENGL;
    SDL_Init(SDL_INIT_VIDEO);
    if (!SDL_CreateWindowAndRenderer("CHIP-8 interpreter", width, height, flags,
                    &window, &renderer)) {
        SDL_LogError(
          SDL_LOG_CATEGORY_APPLICATION,
          "Could not create window and renderer: %s\n",
          SDL_GetError()
        );
        close(fd);
        return EXIT_FAILURE;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING, width, height);
    if (texture == NULL) {
        SDL_LogError(
          SDL_LOG_CATEGORY_APPLICATION,
          "Could not create texture: %s\n",
          SDL_GetError()
        );
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        close(fd);
        return EXIT_FAILURE;
    }

    int debug = 0;
    if (argc == 4)
        debug = 1;

    struct proc_state ps;
    ps.curr_instr = 0;
    ps.pc         = 0;
    ps.err_code   = 0;

    int ret = EXIT_SUCCESS;
    while (!done) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
              case SDL_EVENT_QUIT: // window
                done = true;
                break;
              case SDL_EVENT_KEY_DOWN: // keyboard
                switch (event.key.key) {
                  case SDLK_1: // "1"
                    chip.keyboard[1] = KEY_DOWN;
                    break;
                  case SDLK_2: // "2"
                    chip.keyboard[2] = KEY_DOWN;
                    break;
                  case SDLK_3: // "3"
                    chip.keyboard[3] = KEY_DOWN;
                    break;
                  case SDLK_4: // "C"
                    chip.keyboard[12] = KEY_DOWN;
                    break;
                  case SDLK_Q: // "4"
                    chip.keyboard[4] = KEY_DOWN;
                    break;
                  case SDLK_W: // "5"
                    chip.keyboard[5] = KEY_DOWN;
                    break;
                  case SDLK_E: // "6"
                    chip.keyboard[6] = KEY_DOWN;
                    break;
                  case SDLK_R: // "D"
                    chip.keyboard[13] = KEY_DOWN;
                    break;
                  case SDLK_A: // "7"
                    chip.keyboard[7] = KEY_DOWN;
                    break;
                  case SDLK_S: // "8"
                    chip.keyboard[8] = KEY_DOWN;
                    break;
                  case SDLK_D: // "9"
                    chip.keyboard[9] = KEY_DOWN;
                    break;
                  case SDLK_F: // "E"
                    chip.keyboard[14] = KEY_DOWN;
                    break;
                  case SDLK_Z: // "A"
                    chip.keyboard[10] = KEY_DOWN;
                    break;
                  case SDLK_X: // "0"
                    chip.keyboard[0] = KEY_DOWN;
                    break;
                  case SDLK_C: // "B"
                    chip.keyboard[11] = KEY_DOWN;
                    break;
                  case SDLK_V: // "F"
                    chip.keyboard[15] = KEY_DOWN;
                    break;
                  default: // ignore other keys
                    break;
                }
                break;
              case SDL_EVENT_KEY_UP: // keyboard
                switch (event.key.key) {
                  case SDLK_1: // "1"
                    chip.keyboard[1] = KEY_UP;
                    break;
                  case SDLK_2: // "2"
                    chip.keyboard[2] = KEY_UP;
                    break;
                  case SDLK_3: // "3"
                    chip.keyboard[3] = KEY_UP;
                    break;
                  case SDLK_4: // "C"
                    chip.keyboard[12] = KEY_UP;
                    break;
                  case SDLK_Q: // "4"
                    chip.keyboard[4] = KEY_UP;
                    break;
                  case SDLK_W: // "5"
                    chip.keyboard[5] = KEY_UP;
                    break;
                  case SDLK_E: // "6"
                    chip.keyboard[6] = KEY_UP;
                    break;
                  case SDLK_R: // "D"
                    chip.keyboard[13] = KEY_UP;
                    break;
                  case SDLK_A: // "7"
                    chip.keyboard[7] = KEY_UP;
                    break;
                  case SDLK_S: // "8"
                    chip.keyboard[8] = KEY_UP;
                    break;
                  case SDLK_D: // "9"
                    chip.keyboard[9] = KEY_UP;
                    break;
                  case SDLK_F: // "E"
                    chip.keyboard[14] = KEY_UP;
                    break;
                  case SDLK_Z: // "A"
                    chip.keyboard[10] = KEY_UP;
                    break;
                  case SDLK_X: // "0"
                    chip.keyboard[0] = KEY_UP;
                    break;
                  case SDLK_C: // "B"
                    chip.keyboard[11] = KEY_UP;
                    break;
                  case SDLK_V: // "F"
                    chip.keyboard[15] = KEY_UP;
                    break;
                  default: // ignore other keys
                    break;
                }
                break;
              default: // ignore other events
                break;
            }
        }

        if (done) // do not run one more cycle if user wants to quit
            break;

        // debug mode: waits until lf char is typed before running next cycle
        if (debug)
            getchar();
        run_rom_cycle(&chip, &ps, debug);
        if (ps.err_code > 0) {
            dprintf(STDERR_FILENO, "Error while running ROM, quitting...\n");
            dprintf(
              STDERR_FILENO,
              "[Proc state] instr=%#06x, PC=%#06x, err=%d\n",
              ps.curr_instr, ps.pc, ps.err_code
            );
            ret  = EXIT_FAILURE;
            done = true;
        }

        // scaling, scale must be a power of 2
        uint32_t sc_buf[width * height];
        for (int old_i = 0; old_i < VBUF_HEIGHT; old_i++) {
            for (int old_j = 0; old_j < VBUF_WIDTH; old_j++) {
                int top_left_i = old_i * scale;
                int top_left_j = old_j * scale;
                for (int i = top_left_i; i < top_left_i + scale; i++) {
                    for (int j = top_left_j; j < top_left_j + scale; j++) {
                        uint32_t px = chip.vbuf[old_i * VBUF_WIDTH + old_j];
                        sc_buf[i * width + j] = px;
                    }
                }
            }
        }

        // from sdl doc: the number of bytes in a row of pixel data
        int pitch = sizeof(sc_buf[0]) * width;
        SDL_UpdateTexture(texture, NULL, sc_buf, pitch);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    // Destroy and cleanup
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (close(fd) < 0) {
        perror("close()");
        return EXIT_FAILURE;
    }

    return ret;
}
