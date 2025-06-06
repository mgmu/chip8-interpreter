#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "interpreter.h"

#define INVAL_ARG_ERR "Invalid number of arguments\n"
#define CYCLE_DELAY      16 // Delay in ms between two processor cycles

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        dprintf(STDERR_FILENO, INVAL_ARG_ERR);
        return EXIT_FAILURE;
    }

    // debug mode is enabled if 4 arguments are given
    int debug = 0;
    if (argc == 4)
        debug = 1;

    // Interpreter initialization
    struct interpreter chip;
    init(&chip);
    if (load_rom(argv[1], &chip) < 0) {
        return EXIT_FAILURE;
    }

    // Get scale from arguments
    int scale = atoi(argv[2]);
    if (scale <= 0) {
        dprintf(STDERR_FILENO, "Invalid scale value: %d\n", scale);
        return EXIT_FAILURE;
    }

    // Window and renderer initialization
    SDL_Window      *window;
    SDL_Renderer    *renderer;
    int              width  = VBUF_WIDTH * scale;
    int              height = VBUF_HEIGHT * scale;
    int              ret    = EXIT_SUCCESS;
    SDL_WindowFlags  flags  = SDL_WINDOW_OPENGL;
    SDL_Init(SDL_INIT_VIDEO);
    if (!SDL_CreateWindowAndRenderer("CHIP-8 interpreter", width, height, flags,
                    &window, &renderer)) {
        SDL_LogError(
          SDL_LOG_CATEGORY_APPLICATION,
          "Could not create window and rpenderer: %s\n",
          SDL_GetError()
        );
        ret = EXIT_FAILURE;
        goto clean_up;
    }

    // Processor state initialization
    struct proc_state ps;
    ps.curr_instr = 0;
    ps.pc         = 0;
    ps.err_code   = 0;

    // Processor loop
    bool done = false;
    while (!done) {
        SDL_Delay(CYCLE_DELAY);
        handle_sdl_events(&done, &chip);
        if (done)
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
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return ret;
}
