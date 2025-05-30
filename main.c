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
    int              ret    = EXIT_SUCCESS;
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
        ret = EXIT_FAILURE;
        goto clean_up;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING, width, height);
    if (texture == NULL) {
        SDL_LogError(
          SDL_LOG_CATEGORY_APPLICATION,
          "Could not create texture: %s\n",
          SDL_GetError()
        );
        ret = EXIT_FAILURE;
        goto clean_up;
    }

    int debug = 0;
    if (argc == 4)
        debug = 1;

    struct proc_state ps;
    ps.curr_instr = 0;
    ps.pc         = 0;
    ps.err_code   = 0;


    while (!done) {
        // run rom at 60Hz
        SDL_Delay(16); // 60Hz = 1/60 = 16.67ms
        
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

        run_rom_cycle(&chip, &ps, debug);
        if (ps.err_code > 0) {
            dprintf(STDERR_FILENO, "Error while running ROM, quitting...\n");
            dprintf(
              STDERR_FILENO,
              "[Proc state] instr=%#06x, PC=%#06x, err=%d\n",
              ps.curr_instr, ps.pc, ps.err_code
            );
            ret  = EXIT_FAILURE;
            goto clean_up;
        }

        // set background to black and clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        // draw pixels
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_FRect rect = {.x = 0, .y = 0, .w = scale, .h = scale};
        for (int i = 0; i < VBUF_HEIGHT * VBUF_WIDTH; i++) {
            if (chip.vbuf[i] == PIXEL_ON) {
                rect.x = (i % VBUF_WIDTH) * scale;
                rect.y = (i / VBUF_WIDTH) * scale;
                if (!SDL_RenderFillRect(renderer, &rect)) {
                    SDL_LogError(
                      SDL_LOG_CATEGORY_APPLICATION,
                      "Could not draw pixel: %s\n",
                      SDL_GetError()
                    );
                    ret = EXIT_FAILURE;
                    goto clean_up;
                }
            }
        }
        SDL_RenderPresent(renderer);
    }

    // Destroy and cleanup
 clean_up:
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
