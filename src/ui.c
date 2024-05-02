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
	window = SDL_CreateWindow("lianlipoop", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		printf("SDL_CreateWindow failed err: %s\n", SDL_GetError());
		return -1;
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		printf("SDL_CreateRenderer failed err: %s\n", SDL_GetError());
		return -1;
	}
	TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_LEFT);

	input_box_arr = malloc(sizeof(struct input*) * 10);
	input_box_arr_total_len = 10;
	input_box_arr_used_len = 0;
	selected_input = NULL;

	button_arr = malloc(sizeof(struct button*) * 10);
	button_arr_total_len = 10;
	button_arr_used_len = 0;

	SDL_StartTextInput();
	return 0;
}

void ui_shutdown()
{
	SDL_StopTextInput();
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

void show_screen()
{
	SDL_RenderPresent(renderer);
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
			mouse_button_down(event);
			break;
		case SDL_MOUSEBUTTONUP:
			mouse_button_up(event);
			break;
		case SDL_MOUSEMOTION:
			mouse_move(event);
			break;
		case SDL_KEYDOWN:
			switch (event->key.keysym.sym) {
				case SDLK_ESCAPE:
					running = 0;
					break;

				case SDLK_BACKSPACE:
					if (selected_input != NULL) {
						size_t len = strlen(selected_input->text.str);
						char tmpstr[MAX_TEXT_SIZE];
						strcpy(tmpstr, selected_input->text.str);
						tmpstr[len-1] = '\0';
						change_input_box_text(selected_input, tmpstr);
					}
					break;
				default:
					break;
			}
			break;
		case SDL_TEXTINPUT:
			if (selected_input != NULL) {
				size_t len = strlen(selected_input->text.str);
				char *tmpstr = strncat(selected_input->text.str, event->text.text, MAX_TEXT_SIZE - len);
				change_input_box_text(selected_input, tmpstr);
			}
			break;
		default:
			break;
	}
}

void mouse_move(SDL_Event *event)
{
	if (selected_button != NULL && selected_button->movable) {
		selected_button->function(selected_button, event);
	}
}

void mouse_button_up(SDL_Event *event)
{
	if (selected_button != NULL) {
		selected_button->function(selected_button, event);
		selected_button = NULL;
	} 
	if (selected_input != NULL && selected_input->function != NULL) {
		selected_input->function(selected_input, event);
		selected_input = NULL;
	}
}
void mouse_button_down(SDL_Event *event)
{
	int hit = 0;
	SDL_MouseButtonEvent mouse_data = event->button;
	for (int i = 0; i < input_box_arr_used_len; i++) {
		SDL_Rect box_pos = input_box_arr[i]->outer_box;
		if (mouse_data.x < box_pos.x + box_pos.w && mouse_data.x > box_pos.x 
		 && mouse_data.y < box_pos.y + box_pos.h && mouse_data.y > box_pos.y) {
			hit = 1;
			input_box_arr[i]->selected = 1;
			selected_input = input_box_arr[i];
			SDL_StartTextInput();
		} else if (input_box_arr[i]->selected != 0) {
			input_box_arr[i]->selected = 0;
			SDL_StopTextInput();
		}
	}
	if (!hit) {
		selected_input = NULL;
		for (int i = 0; i < button_arr_used_len; i++) {
			SDL_Rect button_pos = button_arr[i]->outer_box;
			if (mouse_data.x < button_pos.x + button_pos.w && mouse_data.x > button_pos.x 
			 && mouse_data.y < button_pos.y + button_pos.h && mouse_data.y > button_pos.y) {
				selected_button = button_arr[i];
			}

		}
	}
}

SDL_Texture *create_texture_from_surface(SDL_Surface *sur)
{
	SDL_Texture *textest = SDL_CreateTextureFromSurface(renderer, sur);
	if (textest == NULL) {
		printf("create texture did a epic fail\n");
		printf("%s\n", SDL_GetError());
		return NULL;
	}
	return textest;
}

struct text create_text(char *string, int x, int y, int w, int h, int wrap_length, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f)
{
	
	char full_str[MAX_TEXT_SIZE];
	
	struct text new_text = { .str = 0, .show = 0, .wrap_length = wrap_length, .fg_color = fg_color, .bg_color = bg_color, .dst = { x, y, }, };
	
	strncpy(new_text.str, string, MAX_TEXT_SIZE);
	if (w != 0 && h != 0) {
		new_text.static_size = 1;
		new_text.dst.w = w;
		new_text.dst.h = h;
	} else {
		new_text.static_size = 0;
	}
#ifdef INFO
	printf("create_text\nstring = %s\n", string);
	printf("string len = %ld\n", strlen(string));
	printf("static_size = %d\nwrap_length = %d\n\n", new_text.static_size, wrap_length);
#endif
	render_text_texture(&new_text, new_text.fg_color, new_text.bg_color, f);
	return new_text;
}

void destroy_text_texture(struct text *text)
{
	SDL_DestroyTexture(text->texture);
}

void render_text_texture(struct text *t, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f)
{
	SDL_Surface *surface = TTF_RenderUTF8_Shaded_Wrapped(f, t->str, fg_color, bg_color, t->wrap_length);
	t->texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (t->static_size == 0) {
		t->dst.w = surface->w;
		t->dst.h = surface->h;
	}
	
	t->bg_color = bg_color;
	t->fg_color = fg_color;
	SDL_FreeSurface(surface);
}

void change_text_and_render_texture(struct text *text, char *new_text, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f)
{
	if (strlen(new_text) < MAX_TEXT_SIZE) {
		strcpy(text->str, new_text);
		render_text_texture(text, fg_color, bg_color, f);
	}
}

void render_text(struct text *t, SDL_Rect *src)
{
	SDL_RenderCopy(renderer, t->texture, src, &t->dst);
}

struct button *create_button(char *string, int movable, int x, int y, int w, int h, TTF_Font *f, void (*function)(), SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color)
{
	struct button *return_button = malloc(sizeof(struct button));
	struct button new_button = { { 0 }, function, movable, { x, y, }, outer_color, bg_color };

	if (button_arr_used_len + 1 > button_arr_total_len) {
		button_arr = (struct button**)realloc(button_arr, sizeof(struct button*) * button_arr_total_len + 10);
		button_arr_total_len += 10;
	}
	if (string != NULL) {
		new_button.text = create_text(string, x + 5, y + 5, 0, 0, 0, text_color, bg_color, f);
	}
	new_button.outer_box.w = w == 0 ? new_button.text.dst.w + 10 : w;
	new_button.outer_box.h = h == 0 ? new_button.text.dst.h + 10 : h;

	memcpy(return_button, &new_button, sizeof(struct button));
	button_arr[button_arr_used_len] = return_button;
	button_arr_used_len += 1;

	return return_button;
}

void render_button(struct button *button)
{
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(button->bg_color));
	SDL_RenderFillRect(renderer, &button->outer_box);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(button->outer_box_color));
	SDL_RenderDrawRect(renderer, &button->outer_box);
	render_text(&button->text, NULL);
}

// 	   |---------|
// 	-> |some text|
// 	   |---------|
struct input *create_input(char *string, int resize_box, int x, int y, int w, int h, void (*function)(struct input *self, SDL_Event *event), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color)
{
	struct input *return_input = malloc(sizeof(struct input));
	struct input new_input = { .selected = 0, .text = 0, .resize_box = resize_box, .function = function,
					.outer_box = { x, y }, .outer_box_color = outer_color, 
					.bg_color = bg_color };

	if (input_box_arr_used_len + 1 > input_box_arr_total_len) {
		input_box_arr = (struct input**)realloc(input_box_arr, sizeof(struct input*) * input_box_arr_total_len + 10);
		button_arr_total_len += 10;
	}

	if (string != NULL) {
		new_input.text = create_text(string, x+5, y+5, 0, 0, 0, text_color, bg_color, f);
	}

	if (w == 0 && h == 0) {
		new_input.outer_box.w = new_input.text.dst.w + 10;
		new_input.outer_box.h = new_input.text.dst.h + 10;
	}

	if (!resize_box) {
		new_input.text.src = new_input.text.dst;
		new_input.text.src.x = 0;
		new_input.text.src.y = 0;
	}

	memcpy(return_input, &new_input, sizeof(struct input));
	return_input->index = input_box_arr_used_len;
	input_box_arr[input_box_arr_used_len] = return_input;
	input_box_arr_used_len += 1;

	return return_input;
}

//		 |---------|
// some text  -> |some text|
//		 |---------|
struct input *create_input_from_text(struct text text, int resize_box, void (*function)(struct input *self, SDL_Event *event), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color)
{
	struct input *return_input = malloc(sizeof(struct input));
	struct input new_input = { 0, 0, text, resize_box, function, { text.dst.x, text.dst.y, }, outer_color, bg_color };

	if (input_box_arr_used_len + 1 > input_box_arr_total_len) {
		input_box_arr = (struct input**)realloc(input_box_arr, sizeof(struct input*) * input_box_arr_total_len + 10);
	}

	render_text_texture(&new_input.text, text_color, bg_color, f);
	new_input.outer_box.w = new_input.text.dst.w;
	new_input.outer_box.h = new_input.text.dst.h;
	new_input.outer_box.x -= 2;
	new_input.outer_box.y -= 5;
	new_input.outer_box.w += 10;
	new_input.outer_box.h += 10;

	memcpy(return_input, &new_input, sizeof(struct input));
	return_input->index = input_box_arr_used_len;
	input_box_arr[input_box_arr_used_len] = return_input;
	input_box_arr_used_len += 1;

	return return_input;
}

void change_input_box_text(struct input *input_box, char *str)
{
	SDL_Rect prev_input_margin = input_box->outer_box;
	SDL_Rect prev_text_pos = input_box->text.dst;
	prev_input_margin.w = input_box->outer_box.w - input_box->text.dst.w;
	prev_input_margin.h = input_box->outer_box.h - input_box->text.dst.h;

	strncpy(input_box->text.str, str, MAX_TEXT_SIZE);
	render_text_texture(&input_box->text, input_box->text.fg_color, input_box->text.bg_color, font);

	if (input_box->selected) {
		input_box->outer_box.h = input_box->text.dst.h + prev_input_margin.h;
		input_box->outer_box.w = input_box->text.dst.w + prev_input_margin.w;
	}
}

void render_input_box(struct input *input_box)
{
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(input_box->bg_color));
	SDL_RenderFillRect(renderer, &input_box->outer_box);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(input_box->outer_box_color));
	SDL_RenderDrawRect(renderer, &input_box->outer_box);
	if (input_box->resize_box || input_box->selected) render_text(&input_box->text, NULL);
	else {
		//input_box->text.dst.h  = input_box->text.src.h;
		//input_box->text.dst.w  = input_box->text.src.w;
		//input_box->outer_box.h = input_box->text.dst.h + 10;
		//input_box->outer_box.w = input_box->text.dst.w + 10;
		input_box->outer_box.h = input_box->text.src.h + 10;
		input_box->outer_box.w = input_box->text.src.w + 10;
		render_text(&input_box->text, &input_box->text.src);
	}
}

void destroy_input_box(struct input *input_box)
{
	destroy_text_texture(&input_box->text);
	input_box_arr[input_box->index] = NULL;
	free(input_box);
}









