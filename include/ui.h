#ifndef UI_H
#define UI_H
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

const SDL_Color black   = {   0,   0,   0, SDL_ALPHA_OPAQUE };
const SDL_Color red     = { 255,   0,   0, SDL_ALPHA_OPAQUE };
const SDL_Color green   = {   0, 255,   0, SDL_ALPHA_OPAQUE };
const SDL_Color blue    = {   0,   0, 255, SDL_ALPHA_OPAQUE };
const SDL_Color yellow  = { 255, 255,   0, SDL_ALPHA_OPAQUE };

int running;
SDL_Window 	 *window;
SDL_Renderer *renderer;

typedef struct button {
	SDL_Rect pos;
	int clicked;
	const SDL_Color *color;
	void (*funtion)();
} button;

int balen = 3;
button buttonarr[3];

int ui_init();
void mainloop();
void hadleEvent(SDL_Event *e);
void clearscreen();
void ui_shutdown();

#endif
