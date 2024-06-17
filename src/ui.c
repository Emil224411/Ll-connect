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
	window = SDL_CreateWindow("Ll-connect", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_OPENGL);
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

	text_arr = malloc(sizeof(struct text *) * 40);
	text_arr_total_len = 10;
	text_arr_used_len = 0;

	input_box_arr = malloc(sizeof(struct input*) * 10);
	input_box_arr_total_len = 10;
	input_box_arr_used_len = 0;
	selected_input = NULL;

	button_arr = malloc(sizeof(struct button*) * 10);
	button_arr_total_len = 10;
	button_arr_used_len = 0;
	selected_button = NULL;

	ddm_arr = malloc(sizeof(struct drop_down_menu*) * 10);
	ddm_arr_total_len = 10;
	ddm_arr_used_len = 0;
	selected_ddm = NULL;

	slider_arr = malloc(sizeof(struct slider*) * 10);
	slider_arr_total_len = 10;
	slider_arr_used_len = 0;
	selected_slider = NULL;

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
	while (input_box_arr_used_len > 0) {
		destroy_input_box(input_box_arr[input_box_arr_used_len - 1]);
	}
	while (button_arr_used_len > 0) {
		destroy_button(button_arr[button_arr_used_len - 1]);
	}
	while (ddm_arr_used_len > 0) {
		destroy_ddm(ddm_arr[ddm_arr_used_len - 1]);
	}
	while (slider_arr_used_len > 0) {
		destroy_slider(slider_arr[slider_arr_used_len - 1]);
	}
	while (text_arr_used_len > 0) {
		destroy_text(text_arr[text_arr_used_len - 1]);
	}
	free(text_arr);
	free(input_box_arr);
	free(button_arr);
	free(ddm_arr);
	free(slider_arr);
	TTF_Quit();
	SDL_Quit();
}

void show_screen()
{
	SDL_RenderPresent(renderer);
}

void clear_screen(SDL_Color color)
{
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(color));
	SDL_RenderClear(renderer);
}

void handle_event(SDL_Event *event)
{
	switch (event->type) {
		case SDL_QUIT:
			running = 0;
			break;
		case SDL_MOUSEBUTTONDOWN:
			//mabey create a mouse_button_down function for stuff that should happen regardless of left or right
			if (event->button.button == SDL_BUTTON_LEFT) lmouse_button_down(event);
			break;
		case SDL_MOUSEBUTTONUP:
			if (event->button.button == SDL_BUTTON_LEFT) lmouse_button_up(event);
			break;
		case SDL_MOUSEWHEEL:
			mouse_wheel(&event->wheel);
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
						size_t len = strlen(selected_input->text->str);
						char tmpstr[MAX_TEXT_SIZE];
						strcpy(tmpstr, selected_input->text->str);
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
				size_t len = strlen(selected_input->text->str);
				if (len >= selected_input->max_len) break;
				if (selected_input->max_len > 0) {
					char *tmpstr = strncat(selected_input->text->str, event->text.text, MAX_TEXT_SIZE - len);
					change_input_box_text(selected_input, tmpstr);
				}
			}
			break;
		default:
			break;
	}
}

void mouse_wheel(SDL_MouseWheelEvent *event)
{
	int wheely = event->y;
	SDL_Rect mouse_pos = { event->mouseX, event->mouseY, 0 };
	if (selected_ddm != NULL && CHECK_RECT(mouse_pos, selected_ddm->drop_pos) ) {
		int lty = selected_ddm->text[selected_ddm->items-1]->dst.y; 
		int lth = selected_ddm->text[selected_ddm->items-1]->dst.h;
		int dpy = selected_ddm->drop_pos.y, dph = selected_ddm->drop_pos.h;

		selected_ddm->scroll_offset += wheely * 10;
		selected_ddm->update_highlight = 1;
		if (selected_ddm->scroll_offset > 0) 
			selected_ddm->scroll_offset = 0;
		else if (selected_ddm->scroll_offset < -(lty + lth - dpy - dph)) 
			selected_ddm->scroll_offset = -(lty + lth - dpy - dph);

	}
}

void mouse_move(SDL_Event *event)
{
	mouse_x = event->motion.x;
	mouse_y = event->motion.y;
	if (selected_button != NULL && selected_button->movable == 1) {
		selected_button->on_move(selected_button, event);
	}
	if (selected_ddm != NULL && CHECK_RECT(event->motion, selected_ddm->drop_pos)){ 
		selected_ddm->update_highlight = 1;
	}
	if (selected_slider != NULL) {
		update_slider(selected_slider, mouse_x);
		if (selected_slider->on_move) 
			selected_slider->on_move();
	}
}
/* LATER: only check if button or input_box is visible mabey create array with all showen things idk */
void lmouse_button_up(SDL_Event *event)
{
	SDL_MouseButtonEvent mouse_data = event->button;
	if (selected_button != NULL && selected_button->clickable == 1) {
		selected_button->on_click(selected_button, event);
		selected_button = NULL;
	} 
	if (selected_slider != NULL) {
		if (selected_slider->button->on_click != NULL) selected_slider->button->on_click(selected_slider->button, event);
		selected_slider = NULL;

	}
	int hit = 0;
	for (int i = 0; i < input_box_arr_used_len; i++) {
		if (CHECK_RECT(mouse_data, input_box_arr[i]->outer_box))  {
			hit = 1;
			input_box_arr[i]->selected = 1;
			selected_input = input_box_arr[i];
			SDL_StartTextInput();
		} else if (input_box_arr[i]->selected != 0) {
			input_box_arr[i]->selected = 0;
			SDL_StopTextInput();
		}
	}
	if (hit == 0) selected_input = NULL;
	hit = 0;
	if (selected_ddm != NULL && CHECK_RECT(mouse_data, selected_ddm->drop_pos)) {
		for (int i = 0; i < selected_ddm->items; i++) {
			SDL_Rect pos = selected_ddm->text[i]->dst;
			if (selected_ddm->text[i]->show && mouse_data.x < pos.x + selected_ddm->default_pos.w && mouse_data.x > pos.x 
					&& mouse_data.y < pos.y + pos.h + selected_ddm->scroll_offset && mouse_data.y > pos.y + selected_ddm->scroll_offset) {
				selected_ddm->selected_text_index = i;
				selected_ddm->text[i]->show = 1;
				selected_ddm->scroll_offset = -(selected_ddm->text[i]->dst.y - selected_ddm->used_pos.y - 2);
				if (selected_ddm->function != NULL) selected_ddm->function(selected_ddm, event);
				selected_ddm->selected = 0;
				selected_ddm = NULL;
				break;
			}
		}
	} else {
		for (int i = 0; i < ddm_arr_used_len; i++) {
			if (CHECK_RECT(mouse_data, ddm_arr[i]->used_pos)) {
				hit = 1;
				ddm_arr[i]->selected = 1;
				selected_ddm = ddm_arr[i];
			} else if (ddm_arr[i]->selected != 0) {
				ddm_arr[i]->selected = 0;
			}
		}
		if (hit == 0) {
			if (selected_ddm != NULL) selected_ddm->scroll_offset = -(selected_ddm->text[selected_ddm->selected_text_index]->dst.y - selected_ddm->used_pos.y - 2);
			selected_ddm = NULL;
		}
	}
}
void lmouse_button_down(SDL_Event *event)
{
	SDL_MouseButtonEvent mouse_data = event->button;
	for (int i = 0; i < button_arr_used_len; i++) {
		if (button_arr[i]->clickable == 1 && CHECK_RECT(event->button, button_arr[i]->outer_box)) {
			selected_button = button_arr[i];
		}
	}
	for (int i = 0; i < slider_arr_used_len; i++) {
		if (CHECK_RECT(mouse_data, slider_arr[i]->button->outer_box)) {
			selected_slider = slider_arr[i];
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

struct text *create_text(char *string, int x, int y, int w, int h, int font_size, int wrap_length, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f)
{
	char full_str[MAX_TEXT_SIZE];

	if (text_arr_used_len + 1 > text_arr_total_len) {
		text_arr = realloc(text_arr, sizeof(struct text *) * (text_arr_total_len + 10));
		text_arr_total_len += 10;
	}
	
	struct text *return_text = malloc(sizeof(struct text));
	struct text new_text = { .index = text_arr_used_len, .str = 0, .show = 1, .wrap_length = wrap_length, .fg_color = fg_color, .bg_color = bg_color, .dst = { x, y, }, };
	
	strncpy(new_text.str, string, MAX_TEXT_SIZE);
	if (w != 0) {
		new_text.static_w = 1;
		new_text.dst.w = w;
	} 
	if (h != 0) {
		new_text.static_h = 1;
		new_text.dst.h = h;
	}
	new_text.font_size = font_size == 0 ? default_font_size : font_size;
#ifdef INFO
	printf("create_text\nstring = %s\n", string);
	printf("string len = %ld\n", strlen(string));
	printf("wrap_length = %d, new_text.font_size = %d, font_size = %d\n\n", wrap_length, new_text.font_size, font_size);
#endif
	if (render_text_texture(&new_text, new_text.fg_color, new_text.bg_color, f) != 0) {
		printf("ui.c render_text_texture failed from create_text\n");
	}
	memcpy(return_text, &new_text, sizeof(struct text));
	text_arr[text_arr_used_len] = return_text;
	text_arr_used_len += 1;

	return return_text;
}

void destroy_text(struct text *text)
{
	if (text == NULL) return;
	destroy_text_texture(text);
	for (int i = text->index; i < text_arr_used_len-1; i++) {
		text_arr[i] = text_arr[i+1];
		text_arr[i]->index = i;
	}
	text_arr[text_arr_used_len] = NULL;
	text_arr_used_len -= 1;
	free(text);

	printf("text_arr_used_len = %d\n", text_arr_used_len);
}

void destroy_text_texture(struct text *text)
{
	SDL_DestroyTexture(text->texture);
}

int prev_font_size = 20;
int render_text_texture(struct text *t, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f)
{

	if (prev_font_size != t->font_size) {
		TTF_SetFontSize(f, t->font_size);
		prev_font_size = t->font_size;
	}
	SDL_Surface *surface = TTF_RenderUTF8_Shaded_Wrapped(f, t->str, fg_color, bg_color, t->wrap_length);
	if (surface == NULL) {
		printf("ui.c render_text_texture failed surface error: %s\n", SDL_GetError());
		return -1;
	}
	if (t->texture != NULL) SDL_DestroyTexture(t->texture);
	t->texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (t->static_w == 0) {
		t->dst.w = surface->w;
	}
	if (t->static_h == 0) {
		t->dst.h = surface->h;
	}
	t->bg_color = bg_color;
	t->fg_color = fg_color;
	SDL_FreeSurface(surface);
	return 0;
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
	if (t->show != 0) 
		SDL_RenderCopy(renderer, t->texture, src, &t->dst);
}

struct button *create_button(char *string, int movable, int clickable, int show, int x, int y, int w, int h, int font_size, TTF_Font *f, void (*on_click)(), void (*on_move)(), SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color)
{
	struct button *return_button = malloc(sizeof(struct button));
	struct button new_button = { NULL, NULL, on_click, on_move, clickable, movable, show, button_arr_used_len, { x, y, }, outer_color, bg_color };

	if (button_arr_used_len + 1 > button_arr_total_len) {
		button_arr = (struct button**)realloc(button_arr, sizeof(struct button*) * (button_arr_total_len + 5));
		button_arr_total_len += 5;
	}
	if (string != NULL) {
		int tmp_w = 0, tmp_h = 0;
		if (h != 0 && w != 0) tmp_h = h - 10, tmp_w = w - 10;
		new_button.text = create_text(string, x + 5, y + 5, tmp_w, tmp_h, font_size, 0, text_color, bg_color, f);
		new_button.texture = &new_button.text->texture;
	} 
	new_button.outer_box.w = w == 0 ? new_button.text->dst.w + 10 : w;
	new_button.outer_box.h = h == 0 ? new_button.text->dst.h + 10 : h;
	memcpy(return_button, &new_button, sizeof(struct button));
	button_arr[button_arr_used_len] = return_button;
	button_arr_used_len += 1;

	return return_button;
}

void destroy_button(struct button *button)
{
	if (button->text != NULL) destroy_text(button->text);
	for (int i = button->index; i < button_arr_used_len-1; i++) {
		button_arr[i] = button_arr[i+1];
		button_arr[i]->index = i;
	}
	button_arr[button_arr_used_len] = NULL;
	button_arr_used_len -= 1;
	free(button);

	printf("button_arr_used_len = %d\n", button_arr_used_len);
}
void render_button(struct button *button)
{
	if (button->show == 0 ) return;
	else if (button == NULL) {
		printf("error passed a NULL pointer to render_button\n");
		return;
	}
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(button->bg_color));
	SDL_RenderFillRect(renderer, &button->outer_box);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(button->outer_box_color));
	SDL_RenderDrawRect(renderer, &button->outer_box);
	if (button->text != NULL) render_text(button->text, NULL);
}

// 	   |---------|
// 	-> |some text|
// 	   |---------|
struct input *create_input(char *string, int resize_box, int max_len, int x, int y, int w, int h, void (*function)(struct input *self, SDL_Event *event), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color)
{
	struct input *return_input = malloc(sizeof(struct input));
	struct input new_input = { .selected = 0, .text = 0, .resize_box = resize_box, .function = function,
					.default_outer_box = { x, y, }, .outer_box = { x, y }, .outer_box_color = outer_color, 
					.bg_color = bg_color, .max_len = max_len };

	if (input_box_arr_used_len + 1 > input_box_arr_total_len) {
		input_box_arr = (struct input**)realloc(input_box_arr, sizeof(struct input*) * (input_box_arr_total_len + 5));
		button_arr_total_len += 5;
	}

	if (string != NULL) {
		new_input.text = create_text(string, x+5, y+5, 0, 0, 0, 0, text_color, bg_color, f);
	}

	if (w == 0) {
		new_input.default_outer_box.w = new_input.outer_box.w = new_input.text->dst.w + 10;
	} else  new_input.default_outer_box.w = new_input.outer_box.w = w;
	if (h == 0) {
		new_input.default_outer_box.h = new_input.outer_box.h = new_input.text->dst.h + 10;
	} else  new_input.default_outer_box.h = new_input.outer_box.h = h;

	if (!resize_box) {
		new_input.text->src = new_input.text->dst;
		new_input.text->src.x = 0;
		new_input.text->src.y = 0;
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
struct input *create_input_from_text(struct text *text, int resize_box, void (*function)(struct input *self, SDL_Event *event), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color)
{
	struct input *return_input = malloc(sizeof(struct input));
	struct input new_input = { 0, 0, 0, text, resize_box, function, { text->dst.x, text->dst.y }, { text->dst.x, text->dst.y, }, outer_color, bg_color };

	if (input_box_arr_used_len + 1 > input_box_arr_total_len) {
		input_box_arr = (struct input**)realloc(input_box_arr, sizeof(struct input*) * (input_box_arr_total_len + 5));
		input_box_arr_total_len += 5;
	}

	render_text_texture(new_input.text, text_color, bg_color, f);
	new_input.default_outer_box.w = new_input.text->dst.w;
	new_input.default_outer_box.h = new_input.text->dst.h;
	new_input.default_outer_box.x -= 2;
	new_input.default_outer_box.y -= 5;
	new_input.default_outer_box.w += 10;
	new_input.default_outer_box.h += 10;
	new_input.outer_box.w = new_input.text->dst.w;
	new_input.outer_box.h = new_input.text->dst.h;
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
	SDL_Rect prev_text_pos = input_box->text->dst;
	prev_input_margin.w = input_box->outer_box.w - input_box->text->dst.w;
	prev_input_margin.h = input_box->outer_box.h - input_box->text->dst.h;

	strncpy(input_box->text->str, str, MAX_TEXT_SIZE);
	render_text_texture(input_box->text, input_box->text->fg_color, input_box->text->bg_color, font);

	if (input_box->selected && input_box->outer_box.w < input_box->text->dst.w) {
		input_box->outer_box.h = input_box->text->dst.h + prev_input_margin.h;
		input_box->outer_box.w = input_box->text->dst.w + prev_input_margin.w;
	}
}

void render_input_box(struct input *input_box)
{
	if (input_box == NULL) {
		printf("error: passed a NULL pointer to render_input_box");
		return;
	}
	if (input_box->resize_box || input_box->selected) {
		input_box->text->src.x = 0; 
		input_box->text->src.y = 0;
		input_box->text->src.w = input_box->text->dst.w;
		input_box->text->src.h = input_box->text->dst.h;
	} else {
		//why did i comment this out???
		//input_box->text.dst.h  = input_box->default_outer_box.h-10;
		//input_box->text.dst.w  = input_box->default_outer_box.w-10;
		input_box->outer_box.h = input_box->default_outer_box.h;
		input_box->outer_box.w = input_box->default_outer_box.w;
		//input_box->outer_box.h = input_box->text.src.h + 10;
		//input_box->outer_box.w = input_box->text.src.w + 10;
	}
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(input_box->bg_color));
	SDL_RenderFillRect(renderer, &input_box->outer_box);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(input_box->outer_box_color));
	SDL_RenderDrawRect(renderer, &input_box->outer_box);
	render_text(input_box->text, &input_box->text->src);
}

void destroy_input_box(struct input *input_box)
{
	destroy_text(input_box->text);
	for (int i = input_box->index; i < input_box_arr_used_len-1; i++) {
		input_box_arr[i] = input_box_arr[i+1];
		input_box_arr[i]->index = i;
	}
	input_box_arr[input_box_arr_used_len] = NULL;
	input_box_arr_used_len -= 1;
	free(input_box);

	printf("input_box_arr_used_len = %d\n", input_box_arr_used_len);
}

struct drop_down_menu *create_drop_down_menu(int items, char item_str[][MAX_TEXT_SIZE], int x, int y, int w, int h, int dw, int dh, void (*function)(struct drop_down_menu *self, SDL_Event *event), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color tc)
{
	SDL_Rect pos = { x, y, w, h };
	SDL_Rect dpos = { x, y, dw, dh };

	struct text **text_arr = malloc(sizeof(struct text *) * items);
	struct drop_down_menu *ddm_heap = malloc(sizeof(struct drop_down_menu));
	struct drop_down_menu ddm = { 0, 0, 0, ddm_arr_used_len, 1, 1, items, text_arr, function, 0, pos, pos, dpos, {0}, outer_color, bg_color };
	if (pos.h == 0)
		ddm.static_h = 0;

	if (pos.w == 0)
		ddm.static_w = 0;
	ddm_arr_used_len += 1;
	if (ddm_arr_used_len > ddm_arr_total_len) {
		struct drop_down_menu **tmp_ddm_arr = realloc(ddm_arr, sizeof(struct drop_down_menu*) * (ddm_arr_total_len + 5));
		if (tmp_ddm_arr != NULL) ddm_arr = tmp_ddm_arr;
		ddm_arr_total_len += 5;
	}
	int prev_height = 0;
	int bigest_w = ddm.default_pos.w, bigest_h = ddm.default_pos.h;
	for (int i = 0; i < items; i++) {
		ddm.text[i] = create_text(item_str[i], 0, 0, 0, 0, 0, w, tc, bg_color, f);
		if (!ddm.static_w && bigest_w < ddm.text[i]->dst.w) bigest_w = ddm.text[i]->dst.w;
		if (!ddm.static_h && bigest_h < ddm.text[i]->dst.h) bigest_h = ddm.text[i]->dst.h;
		ddm.text[i]->dst.y = ddm.default_pos.y + prev_height + 2;
		ddm.text[i]->dst.x = ddm.default_pos.x + 4;
		prev_height += ddm.text[i]->dst.h;
		ddm.text[i]->show = 0;
	}
	ddm.text[0]->show = 1;

	if (!ddm.static_w) ddm.used_pos.w = ddm.default_pos.w = bigest_w + 8;
	if (dpos.w == 0) ddm.drop_pos.w = bigest_w + 8;
	if (!ddm.static_h) ddm.used_pos.h = ddm.default_pos.h = ddm.text[0]->dst.h + 4;
	if (dpos.h == 0) ddm.drop_pos.h = ddm.text[0]->dst.h + 4;

	//printf("create dmm\nddm.default_pos = { %d, %d, %d, %d }, ddm.used_pos = { %d, %d, %d, %d }, ddm.drop_pos = { %d, %d, %d, %d }\n", 
	//		ddm.default_pos.x, ddm.default_pos.y, ddm.default_pos.w, ddm.default_pos.h, ddm.used_pos.x, ddm.used_pos.y, ddm.used_pos.w, ddm.used_pos.h,
	//		ddm.drop_pos.x, ddm.drop_pos.y, ddm.drop_pos.w, ddm.drop_pos.h);

	memcpy(ddm_heap, &ddm, sizeof(struct drop_down_menu));
	ddm_arr[ddm_arr_used_len-1] = ddm_heap;
	return ddm_arr[ddm_arr_used_len-1];
}

void change_ddm_text_arr(struct drop_down_menu *ddm, int items, char newstr[][MAX_TEXT_SIZE], TTF_Font *f)
{
	for (int i = 0; i < ddm->items; i++) {
		destroy_text(ddm->text[i]);
	}
	ddm->text = realloc(ddm->text, sizeof(struct text *) * items);

	SDL_Color tc = ddm->text[0]->fg_color;
	int prev_height = 0;
	int bigest_w = ddm->default_pos.w, bigest_h = ddm->default_pos.h;
	for (int i = 0; i < items; i++) {
		ddm->text[i] = create_text(newstr[i], 0, 0, 0, 0, 0, ddm->default_pos.w, tc, ddm->bg_color, f);
		printf("str = %s\n", newstr[i]);
		if (!ddm->static_w && bigest_w < ddm->text[i]->dst.w) bigest_w = ddm->text[i]->dst.w;
		if (!ddm->static_h && bigest_h < ddm->text[i]->dst.h) bigest_h = ddm->text[i]->dst.h;
		ddm->text[i]->dst.y = ddm->default_pos.y + prev_height + 2;
		ddm->text[i]->dst.x = ddm->default_pos.x + 4;
		prev_height += ddm->text[i]->dst.h;
		ddm->text[i]->show = 0;
	}
	ddm->items = items;
	ddm->selected_text_index = 0;
	ddm->scroll_offset = 0;
	ddm->text[0]->show = 1;
	/*printf("create dmm\nddm.default_pos = { %d, %d, %d, %d }, ddm.used_pos = { %d, %d, %d, %d }, ddm.drop_pos = { %d, %d, %d, %d }\n", 
			ddm->default_pos.x, ddm->default_pos.y, ddm->default_pos.w, ddm->default_pos.h, ddm->used_pos.x, ddm->used_pos.y, ddm->used_pos.w, ddm->used_pos.h,
			ddm->drop_pos.x, ddm->drop_pos.y, ddm->drop_pos.w, ddm->drop_pos.h);
			*/
}

void destroy_ddm(struct drop_down_menu *ddm) 
{
	for (int i = 0; i < ddm->items; i++) {
		destroy_text(ddm->text[i]);
	}
	for (int i = ddm->index; i < ddm_arr_used_len-1; i++) {
		ddm_arr[i] = ddm_arr[i+1];
		ddm_arr[i]->index = i;
	}
	ddm_arr[ddm_arr_used_len] = NULL;
	ddm_arr_used_len -= 1;
	free(ddm);

	printf("ddm_arr_used_len = %d\n", ddm_arr_used_len);
}

/* 	mabey split up in to a few functions but idk 	*/
void render_ddm(struct drop_down_menu *ddm)
{
	if (ddm == NULL) {
		printf("error passed a NULL pointer to render_ddm\n");
		return;
	}
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(ddm->bg_color));
	if (ddm->selected == 0) {
		ddm->used_pos = ddm->default_pos;

		if (ddm->text[ddm->selected_text_index]->dst.h >= ddm->used_pos.h) 
			ddm->used_pos.h = ddm->text[ddm->selected_text_index]->dst.h + 2;

		SDL_RenderFillRect(renderer, &ddm->used_pos);

		for (int i = 0; i < ddm->items; i++) ddm->text[i]->show = i == ddm->selected_text_index ? 1 : 0;

		struct text tmp_text = { .texture = ddm->text[ddm->selected_text_index]->texture, .static_h = 0, .show = 1,
					 .static_w = 0, .dst = ddm->text[ddm->selected_text_index]->dst, .src = { 0, 0, ddm->text[ddm->selected_text_index]->dst.w,  ddm->text[ddm->selected_text_index]->dst.h } };

		tmp_text.dst.x = ddm->default_pos.x + 4;
		tmp_text.dst.y = ddm->default_pos.y + 2;
		if (tmp_text.dst.x + tmp_text.dst.w > ddm->drop_pos.x + ddm->drop_pos.w) {
			tmp_text.src.w -= (tmp_text.dst.x + tmp_text.dst.w) - (ddm->default_pos.x + ddm->default_pos.w);
			tmp_text.dst.w -= (tmp_text.dst.x + tmp_text.dst.w) - (ddm->default_pos.x + ddm->default_pos.w);
		}

		render_text(&tmp_text, &tmp_text.src);

		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(ddm->outer_box_color));
		SDL_RenderDrawRect(renderer, &ddm->used_pos);
	} else {
		SDL_RenderFillRect(renderer, &ddm->drop_pos);
		int too_big = 0;
		for (int i = 0; i < ddm->items && !too_big; i++) {
			ddm->text[i]->show = 1;
			struct text tmp_text = { .show = 1, .texture = ddm->text[i]->texture, 
					.dst = ddm->text[i]->dst, .src = { 0, 0, ddm->text[i]->dst.w, ddm->text[i]->dst.h} };

			tmp_text.dst.y += ddm->scroll_offset;
			if (tmp_text.dst.x + tmp_text.dst.w > ddm->drop_pos.x + ddm->drop_pos.w) {
				tmp_text.src.w -= (tmp_text.dst.x + tmp_text.dst.w) - (ddm->default_pos.x + ddm->default_pos.w);
				tmp_text.dst.w -= (tmp_text.dst.x + tmp_text.dst.w) - (ddm->default_pos.x + ddm->default_pos.w);
			}
			if (tmp_text.dst.y + tmp_text.dst.h > ddm->default_pos.y && tmp_text.dst.y < ddm->default_pos.y) {
				tmp_text.src.y = ddm->default_pos.y - tmp_text.dst.y;
				tmp_text.src.h = tmp_text.dst.h = (tmp_text.dst.y + tmp_text.dst.h) - ddm->default_pos.y;
				tmp_text.dst.y = ddm->default_pos.y;
				render_text(&tmp_text, &tmp_text.src);
			} else if (tmp_text.dst.y + tmp_text.dst.h > ddm->default_pos.y + ddm->drop_pos.h) {
				too_big = 1;
				tmp_text.src.h = tmp_text.dst.h = (ddm->default_pos.y + ddm->drop_pos.h) - tmp_text.dst.y;
				render_text(&tmp_text, &tmp_text.src);
			} else if (tmp_text.dst.y + tmp_text.dst.h > ddm->default_pos.y) {
				render_text(&tmp_text, &tmp_text.src);
			} else ddm->text[i]->show = 0;
			ddm->text[i]->src = tmp_text.src;
		}
		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(ddm->outer_box_color));
		SDL_RenderDrawRect(renderer, &ddm->drop_pos);
		if (ddm->update_highlight) update_ddm_highlight(mouse_x, mouse_y, ddm);
		SDL_RenderDrawRect(renderer, &ddm->highlight_pos);
	}
}

void update_ddm_highlight(int x, int y, struct drop_down_menu *ddm)
{
	ddm->highlight_pos.x = ddm->default_pos.x + 1;
	for (int i = 0; i < ddm->items; i++) {
		if (ddm->text[i]->show && ddm->text[i]->dst.y + ddm->scroll_offset <= y && 
				ddm->text[i]->dst.y + ddm->text[i]->dst.h + ddm->scroll_offset >= y) {
			ddm->highlight_pos.y = ddm->text[i]->dst.y + ddm->text[i]->src.y + ddm->scroll_offset;
			ddm->highlight_pos.h = ddm->text[i]->src.h;
			break;
		}
	}
	ddm->highlight_pos.w = ddm->default_pos.w - 2;
}


struct slider *create_slider(int show, int bar_x, int bar_y, int bar_w, int bar_h, int button_size, void (*on_relase)(), void (*on_move)(), SDL_Color button_fg_color, SDL_Color button_bg_color, SDL_Color bar_color)
{
	struct slider *return_slider = malloc(sizeof(struct slider));
	struct button *b = create_button(NULL, 1, 0, show, bar_x - button_size/2, bar_y + bar_h/2- button_size/2, button_size, button_size, 0, font, on_relase, update_slider, button_fg_color, button_bg_color, button_fg_color);
	struct slider new_slider = { b, { bar_x, bar_y, bar_w, bar_h }, 0, show, slider_arr_used_len, bar_color, on_move };

	if (slider_arr_used_len + 1 > slider_arr_total_len) {
		slider_arr = (struct slider**)realloc(slider_arr, sizeof(struct slider*) * (slider_arr_total_len + 5));
		slider_arr_total_len += 5;
	}
	memcpy(return_slider, &new_slider, sizeof(struct slider));
	slider_arr[slider_arr_used_len] = return_slider;
	slider_arr_used_len += 1;

	return slider_arr[slider_arr_used_len-1];
}

void destroy_slider(struct slider *slider)
{
	printf("slider_arr_used_len = %d\n", slider_arr_used_len);
	for (int i = 0; i < slider_arr_used_len; i++) {
		printf("slider_arr[%d].index = %d\n", i, slider_arr[i]->index);
	}

	for (int i = slider->index; i < slider_arr_used_len-1; i++) {
		slider_arr[i] = slider_arr[i+1];
		slider_arr[i]->index = i;
	}
	slider_arr[slider_arr_used_len] = NULL;
	slider_arr_used_len -= 1;
	free(slider);

	printf("slider_arr_used_len = %d\n", slider_arr_used_len);
	for (int i = 0; i < slider_arr_used_len; i++) {
		printf("slider_arr[%d].index = %d\n", i, slider_arr[i]->index);
	}
}

void render_slider(struct slider *slider)
{
	if (slider == NULL) {
		printf("error passed a NULL pointer to render_slider\n");
		return;
	}
	if (!slider->show) return;
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(slider->bar_color));
	SDL_RenderFillRect(renderer, &slider->pos);
	render_button(slider->button);
}

void update_slider(struct slider *slider, int x) 
{
	if      (x >= slider->pos.x + slider->pos.w) x = slider->pos.x + slider->pos.w;
	else if (x <= slider->pos.x) 		     x = slider->pos.x;
	slider->p = (float)(x-slider->pos.x)/(slider->pos.w);
	slider->button->outer_box.x = x - slider->button->outer_box.w/2;
}













