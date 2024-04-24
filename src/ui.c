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

	input_box_arr = malloc(sizeof(struct input*)*10);
	input_box_arr_total_len = 10;
	input_box_arr_used_len = 0;

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
	for (int i = 0; i < input_box_arr_used_len; i++) {
		destroy_input_box(input_box_arr[i]);
		input_box_arr[i] = NULL;
	}
	free(input_box_arr);
	TTF_Quit();
	SDL_Quit();
}

void clear_screen()
{
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(BLACK));
	SDL_RenderClear(renderer);
}

void handle_event(SDL_Event *event)
{
	switch (event->type) {
		case SDL_QUIT:
			running = 0;
			break;
		case SDL_MOUSEBUTTONDOWN:
			mousebutton(event->button);
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

void mousebutton(SDL_MouseButtonEvent mouse_data)
{
	for (int i = 0; i < input_box_arr_used_len; i++) {
		SDL_Rect box_pos = input_box_arr[i]->outer_box;
		if (mouse_data.x < box_pos.x + box_pos.w && mouse_data.x > box_pos.x 
		 && mouse_data.y < box_pos.y + box_pos.h && mouse_data.y > box_pos.y) {
			input_box_arr[i]->selected = 1;
			printf("input_box selected\n");
		} else if (input_box_arr[i]->selected != 0) {
			input_box_arr[i]->selected = 0;
			printf("input_box deselected\n");
		}
	}
}

struct text create_text(char *string, int x, int y, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f)
{
	struct text new_text = { string, 0, 0, fg_color, bg_color, { x, y, }, };
	SDL_Surface *surface = TTF_RenderText_Shaded(f, string, fg_color, bg_color);
	new_text.texture = SDL_CreateTextureFromSurface(renderer, surface);
	TTF_SizeText(f, string, &new_text.pos.w, &new_text.pos.h);
	SDL_FreeSurface(surface);
	return new_text;
}

void destroy_text_texture(struct text *text)
{
	SDL_DestroyTexture(text->texture);
}

void render_text_texture(struct text *t, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f)
{
	SDL_Surface *surface = TTF_RenderText_Shaded(f, t->str, fg_color, bg_color);
	t->texture = SDL_CreateTextureFromSurface(renderer, surface);
	TTF_SizeText(f, t->str, &t->pos.w, &t->pos.h);
	t->bg_color = bg_color;
	t->fg_color = fg_color;
	SDL_FreeSurface(surface);
}

void change_text_and_render_texture(struct text *text, char *new_text, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f)
{
	text->str = new_text;
	render_text_texture(text, fg_color, bg_color, f);
}

void render_text(struct text *t)
{
	SDL_RenderCopy(renderer, t->texture, NULL, &t->pos);
}

//		 |---------|
// some text  -> |some text|
//		 |---------|
struct input *create_input_from_text(struct text text, TTF_Font *f, SDL_Color outer_color, SDL_Color background_color, SDL_Color text_color)
{

	struct input new_input = { 0, 0, text, { text.pos.x, text.pos.y, }, outer_color, background_color };
	render_text_texture(&new_input.text, text_color, background_color, f);
	TTF_SizeText(f, new_input.text.str, &new_input.outer_box.w, &new_input.outer_box.h);
	new_input.outer_box.x -= 2;
	new_input.outer_box.y -= 5;
	new_input.outer_box.w += 10;
	new_input.outer_box.h += 10;
	if (input_box_arr_used_len + 1 > input_box_arr_total_len) {
		input_box_arr = (struct input**)realloc(input_box_arr, sizeof(struct input*) * input_box_arr_total_len + 10);
	}
	struct input *return_input = malloc(sizeof(struct input));
	memcpy(return_input, &new_input, sizeof(struct input));
	return_input->index = input_box_arr_used_len;
	input_box_arr[input_box_arr_used_len] = return_input;
	input_box_arr_used_len += 1;
	return return_input;
}

// 	   |---------|
// 	-> |some text|
// 	   |---------|
struct input *create_input(char *text, int x, int y, int w, int h, TTF_Font *f, SDL_Color outer_color, SDL_Color background_color, SDL_Color text_color)
{
	struct input new_input = { .selected = 0, .text = NULL, .outer_box = { x, y }, .outer_box_color = outer_color, .background_color = background_color };
	if (text != NULL) {
		new_input.text = create_text(text, x+5, y+5, text_color, background_color, f);
	}

	if (w == 0 && h == 0) {
		new_input.outer_box.w = new_input.text.pos.w + 10;
		new_input.outer_box.h = new_input.text.pos.h + 10;
	}
	if (input_box_arr_used_len + 1 > input_box_arr_total_len) {
		input_box_arr = (struct input**)realloc(input_box_arr, sizeof(struct input*) * input_box_arr_total_len + 10);
	}
	struct input *return_input = malloc(sizeof(struct input));
	memcpy(return_input, &new_input, sizeof(struct input));
	return_input->index = input_box_arr_used_len;
	input_box_arr[input_box_arr_used_len] = return_input;
	input_box_arr_used_len += 1;
	return return_input;
}

void render_input_box(struct input *input_box)
{
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(input_box->background_color));
	SDL_RenderFillRect(renderer, &input_box->outer_box);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(input_box->outer_box_color));
	SDL_RenderDrawRect(renderer, &input_box->outer_box);
	render_text(&input_box->text);
}

void destroy_input_box(struct input *input_box)
{
	destroy_text_texture(&input_box->text);
	input_box_arr[input_box->index] = NULL;
	free(input_box);
}









