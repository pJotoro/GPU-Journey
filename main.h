#pragma once

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#define assert(expr) SDL_assert(expr)

#define CHECK(RES, EXPR) RES = EXPR; assert(RES)

struct Game {
	SDL_Window* window;
};