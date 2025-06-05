#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "interpreter.h"

#define MISSING_FILE_ERR "Missing file name\n"
#define CYCLE_DELAY      16 // Delay in ms between two processor cycles

// Handles window and keyboard events, and udpates done and chip accordingly.
void handle_sdl_events(bool *done, struct interpreter *chip) {
    if (done == NULL || chip == NULL)
        return;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
          case SDL_EVENT_QUIT: // window
            *done = true;
            break;
          case SDL_EVENT_KEY_DOWN: // keyboard
            switch (event.key.key) {
              case SDLK_1: // "1"
                chip->keyboard[1] = KEY_DOWN;
                break;
              case SDLK_2: // "2"
                chip->keyboard[2] = KEY_DOWN;
                break;
              case SDLK_3: // "3"
                chip->keyboard[3] = KEY_DOWN;
                break;
              case SDLK_4: // "C"
                chip->keyboard[12] = KEY_DOWN;
                break;
              case SDLK_Q: // "4"
                chip->keyboard[4] = KEY_DOWN;
                break;
              case SDLK_W: // "5"
                chip->keyboard[5] = KEY_DOWN;
                break;
              case SDLK_E: // "6"
                chip->keyboard[6] = KEY_DOWN;
                break;
              case SDLK_R: // "D"
                chip->keyboard[13] = KEY_DOWN;
                break;
              case SDLK_A: // "7"
                chip->keyboard[7] = KEY_DOWN;
                break;
              case SDLK_S: // "8"
                chip->keyboard[8] = KEY_DOWN;
                break;
              case SDLK_D: // "9"
                chip->keyboard[9] = KEY_DOWN;
                break;
              case SDLK_F: // "E"
                chip->keyboard[14] = KEY_DOWN;
                break;
              case SDLK_Z: // "A"
                chip->keyboard[10] = KEY_DOWN;
                break;
              case SDLK_X: // "0"
                chip->keyboard[0] = KEY_DOWN;
                break;
              case SDLK_C: // "B"
                chip->keyboard[11] = KEY_DOWN;
                break;
              case SDLK_V: // "F"
                chip->keyboard[15] = KEY_DOWN;
                break;
              default: // ignore other keys
                break;
            }
            break;
          case SDL_EVENT_KEY_UP: // keyboard
            switch (event.key.key) {
              case SDLK_1: // "1"
                chip->keyboard[1] = KEY_UP;
                break;
              case SDLK_2: // "2"
                chip->keyboard[2] = KEY_UP;
                break;
              case SDLK_3: // "3"
                chip->keyboard[3] = KEY_UP;
                break;
              case SDLK_4: // "C"
                chip->keyboard[12] = KEY_UP;
                break;
              case SDLK_Q: // "4"
                chip->keyboard[4] = KEY_UP;
                break;
              case SDLK_W: // "5"
                chip->keyboard[5] = KEY_UP;
                break;
              case SDLK_E: // "6"
                chip->keyboard[6] = KEY_UP;
                break;
              case SDLK_R: // "D"
                chip->keyboard[13] = KEY_UP;
                break;
              case SDLK_A: // "7"
                chip->keyboard[7] = KEY_UP;
                break;
              case SDLK_S: // "8"
                chip->keyboard[8] = KEY_UP;
                break;
              case SDLK_D: // "9"
                chip->keyboard[9] = KEY_UP;
                break;
              case SDLK_F: // "E"
                chip->keyboard[14] = KEY_UP;
                break;
              case SDLK_Z: // "A"
                chip->keyboard[10] = KEY_UP;
                break;
              case SDLK_X: // "0"
                chip->keyboard[0] = KEY_UP;
                break;
              case SDLK_C: // "B"
                chip->keyboard[11] = KEY_UP;
                break;
              case SDLK_V: // "F"
                chip->keyboard[15] = KEY_UP;
                break;
              default: // ignore other keys
                break;
            }
            break;
          default: // ignore other events
            break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        dprintf(STDERR_FILENO, MISSING_FILE_ERR);
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

    // Window and renderer initialization
    SDL_Window      *window;
    SDL_Renderer    *renderer;
    int              scale  = atoi(argv[2]);
    int              width  = VBUF_WIDTH * scale;
    int              height = VBUF_HEIGHT * scale;
    int              ret    = EXIT_SUCCESS;
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

