#include "../include/ui.h"

int ui_init() 
{
	window = NULL;
	renderer = NULL;
	if (SDL_Init(SDL_INIT_EVERYTHING)) {
		printf("SDL_Init failed err: %s\n", SDL_GetError());
		return 1;
	}

	window = SDL_CreateWindow("lianlipoop", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 400, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		printf("SDL_CreateWindow failed err: %s\n", SDL_GetError());
		return 1;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		printf("SDL_CreateRenderer failed err: %s\n", SDL_GetError());
		return 1;
	}

	return 0;
	
}

void ui_shutdown()
{
	if (window != NULL) {
		SDL_DestroyWindow(window);
		window = NULL;
	}
	if (renderer != NULL) {
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}

	SDL_Quit();
}

void clearscreen()
{
	SDL_SetRenderDrawColor(renderer, black.r, black.g, black.b, black.a);
	SDL_RenderClear(renderer);
}
void mouseclick(SDL_MouseButtonEvent *me)
{
	if (me->button == SDL_BUTTON_LEFT) {
		for (int i = 0; i < balen; i++) {
			if (me->x > buttonarr[i].pos.x && me->x < buttonarr[i].pos.x + buttonarr[i].pos.w 
				&& me->y > buttonarr[i].pos.y && me->y < buttonarr[i].pos.y + buttonarr[i].pos.h) {
				buttonarr[i].funtion();
			}
		}
	}
}
void hadleEvent(SDL_Event *e)
{
	switch (e->type) {
		case SDL_MOUSEBUTTONDOWN:
			mouseclick(&e->button);
			break;
		case SDL_KEYDOWN:
			switch (e->key.keysym.scancode) {
				case SDL_SCANCODE_ESCAPE:
					running = 0;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void drawButtons()
{
	for (int i = 0; i < balen; i++) {
		SDL_SetRenderDrawColor(renderer, buttonarr[i].color->r, buttonarr[i].color->g, buttonarr[i].color->b, buttonarr[i].color->a);
		SDL_RenderDrawRect(renderer, &buttonarr[i].pos);
	}
}

void mainloop()
{
	running = 1;
	SDL_Event event;

	while (running) {
		clearscreen();

		if (SDL_PollEvent(&event)) {
			hadleEvent(&event);
		}
		drawButtons();
		SDL_RenderPresent(renderer);
	}
	ui_shutdown();
}
