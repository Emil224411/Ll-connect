#include "../include/ui.h"

int ui_init() 
{
	window = NULL;
	renderer = NULL;
	if (SDL_Init(SDL_INIT_EVERYTHING)) {
		printf("SDL_Init failed err: %s\n", SDL_GetError());
		return -1;
	}
	if (TTF_Init()) {
		printf("TTF_Init failed err: %s\n", TTF_GetError());
		return -1;
	}
	font = TTF_OpenFont(font_path, 20);
	if (font == NULL) {
		printf("TTF_OpenFont failed error: %s\n", TTF_GetError());
		return -1;
	}
	window = SDL_CreateWindow("lianlipoop", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 400, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		printf("SDL_CreateWindow failed err: %s\n", SDL_GetError());
		return -1;
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		printf("SDL_CreateRenderer failed err: %s\n", SDL_GetError());
		return -1;
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
	if (font != NULL) {
		TTF_CloseFont(font);
		font = NULL;
	}
	TTF_Quit();
	SDL_Quit();
}

void clear_screen()
{
	SDL_SetRenderDrawColor(renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
	SDL_RenderClear(renderer);
}

void handle_event(SDL_Event *event)
{
	switch (event->type) {
		case SDL_QUIT:
			running = 0;
			break;
		case SDL_MOUSEBUTTONDOWN:
			break;
		case SDL_KEYDOWN:
			switch (event->key.keysym.scancode) {
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

struct text create_text(char *string, int x, int y, SDL_Color color, TTF_Font *f)
{
	struct text new_text = { string, 0, 0, { x, y, }, };
	SDL_Surface *surface = TTF_RenderText_Solid(f, string, color);
	new_text.texture = SDL_CreateTextureFromSurface(renderer, surface);
	TTF_SizeText(f, string, &new_text.pos.w, &new_text.pos.h);
	SDL_FreeSurface(surface);
	return new_text;
}

void destroy_text_texture(struct text *text)
{
	SDL_DestroyTexture(text->texture);
}

void render_text_texture(struct text *t, SDL_Color color, TTF_Font *f)
{
	SDL_Surface *surface = TTF_RenderText_Solid(f, t->str, color);
	t->texture = SDL_CreateTextureFromSurface(renderer, surface);
	TTF_SizeText(f, t->str, &t->pos.w, &t->pos.h);
	SDL_FreeSurface(surface);
}

void change_text_and_render_texture(struct text *text, char *new_text, SDL_Color color, TTF_Font *f)
{
	text->str = new_text;
	render_text_texture(text, color, f);
}

void render_text(struct text *t)
{
	SDL_RenderCopy(renderer, t->texture, NULL, &t->pos);
}

//TODO: 
//current:
// 		       |---------|
// 	 some text  -> |some text|
// 		       |---------|
//better:
// 	|---------|    |---------|
// 	| 	  | -> |some text|
// 	|---------|    |---------|
// 	make new function create_input and create_input_from_text or something like that
struct input create_input(struct text text, TTF_Font *f, SDL_Color outer_color, SDL_Color background_color, SDL_Color text_color)
{

	struct input new_input = { 0, text, { text.pos.x, text.pos.y, }, outer_color, background_color, text_color };
	if (new_input.text.texture == NULL) {
		render_text_texture(&new_input.text, text_color, f);
	}
		TTF_SizeText(f, new_input.text.str, &new_input.outer_box.w, &new_input.outer_box.h);
		new_input.outer_box.x -= 2;
		new_input.outer_box.y -= 5;
		new_input.outer_box.w += 10;
		new_input.outer_box.h += 10;
	return new_input;
}

void render_input_box(struct input *input_box)
{
	SDL_SetRenderDrawColor(renderer, input_box->background_color.r, input_box->background_color.g, input_box->background_color.b, input_box->background_color.a);
	SDL_RenderFillRect(renderer, &input_box->outer_box);
	SDL_SetRenderDrawColor(renderer, input_box->outer_box_color.r, input_box->outer_box_color.g, input_box->outer_box_color.b, input_box->outer_box_color.a);
	SDL_RenderDrawRect(renderer, &input_box->outer_box);
	render_text(&input_box->text);
}









