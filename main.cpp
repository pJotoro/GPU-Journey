#include "main.h"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	bool ok;

	CHECK(ok, SDL_Init(SDL_INIT_VIDEO));

	Game* game = new Game;

	SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
	// TODO
#ifdef _DEBUG
	int w = 1920;
	int h = 1080;
	bool debug_mode = true;
#else
	int w = 3840;
	int h = 2160;
	bool debug_mode = false;
	flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALWAYS_ON_TOP;
#endif
	CHECK(game->window, SDL_CreateWindow("GPU Journey", w, h, flags));

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	Game* game = static_cast<Game*>(appstate);


	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	Game* game = static_cast<Game*>(appstate);

	switch (event->type) {
	case SDL_EVENT_KEY_DOWN:
		if (event->key.key == SDLK_ESCAPE) {
			return SDL_APP_SUCCESS;
		}
		break;
	case SDL_EVENT_QUIT:
		return SDL_APP_SUCCESS;
	}
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {

}