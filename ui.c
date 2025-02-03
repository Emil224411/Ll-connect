#include "ui.h"

int running;

SDL_Window   *window;
SDL_Renderer *renderer;
TTF_Font     *font;
char font_path[128];
int default_font_size = 20;
struct line *cursor;

color BLACK   = {   0,   0,   0, SDL_ALPHA_OPAQUE };
color RED     = { 255,   0,   0, SDL_ALPHA_OPAQUE };
color GREEN   = {   0, 255,   0, SDL_ALPHA_OPAQUE };
color BLUE    = {   0,   0, 255, SDL_ALPHA_OPAQUE };
color WHITE   = { 255, 255, 255, SDL_ALPHA_OPAQUE };

static int mouse_x, mouse_y;

static struct callback *callback_arr;
static int callback_amount = 0, callback_total = 0;
static struct callback **callback_que;
static int que_len = 0, que_total = 0;
static struct prompt **prompt_arr;
struct prompt *showen_prompt = NULL;
static int prompt_arr_len;
static struct page **page_arr;
struct page *showen_page;
static int page_arr_total_len;
static int page_arr_used_len;

static void lmouse_button_down(Event *event);
static void lmouse_button_up(Event *event);
static void on_backspace_input(struct input *selected);
static void on_text_input(struct input *selected, SDL_TextInputEvent *e);
static void rmouse_button_down(Event *event);
static void rmouse_button_up(Event *event);
static void mouse_wheel(SDL_MouseWheelEvent *event);
static void mouse_move(Event *event);

static struct callback *next_cb;

void delay(Uint32 ms)
{
	SDL_Delay(ms);
}

surface_s *create_RGB_surface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	return SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);
}

void destroy_texture_s(texture_s *t)
{
	SDL_DestroyTexture(t);
}

Uint32 get_ticks(void) 
{
	return SDL_GetTicks();
}

struct callback *create_callback(void (*function)(void), double time)
{
	PRINTINFO("create_callback\n");
	if (callback_amount + 1 > callback_total) {
		struct callback *tmp = realloc(callback_arr, sizeof(struct callback) * (callback_total + 5));
		if (tmp == NULL) {
			printf("failed to reallocate callback_arr\n");
			return NULL;
		}
		callback_arr = tmp;
		callback_total += 5;
	}
	callback_arr[callback_amount].function = function;
	callback_arr[callback_amount].a = SDL_GetTicks();
	callback_arr[callback_amount].b = SDL_GetTicks();
	callback_arr[callback_amount].timer = time;
	callback_arr[callback_amount].index = callback_amount;
	callback_arr[callback_amount].is_qued = 0;
	callback_arr[callback_amount].times_called_back = 0;
	callback_amount++;
	return &callback_arr[callback_amount - 1];
}

void add_callback_to_que(struct callback *cb)
{
	if (!cb->is_qued) {
		if (que_len + 1 > que_total) {
			cb->times_called_back = 0;
			struct callback **tmp = realloc(callback_que, sizeof(struct callback *) * (que_total + 5));
			if (tmp == NULL) {
				printf("add_callback_to_que: failed to reallocate callback_que\n");
				return;
			}
			callback_que = tmp;
			que_total += 5;
		}
		cb->is_qued = 1;
		cb->b = SDL_GetTicks();
		callback_que[que_len] = cb;
		if (next_cb == NULL || get_time_till_cb(callback_que[que_len]) < get_time_till_cb(next_cb))
			next_cb = callback_que[que_len];
		que_len++;
	}
}

double get_time_till_cb(struct callback *cb)
{
	cb->a = SDL_GetTicks();
	return cb->timer - (cb->a - cb->b);
}

void set_callback_timer(struct callback *cb, double time)
{
	cb->timer = time;
	if (next_cb == NULL || get_time_till_cb(cb) < get_time_till_cb(next_cb)) {
		next_cb = cb;
	}

}

void destroy_callback(struct callback *cb)
{
	for (int i = cb->index; i < callback_amount - 1; i++) {
		callback_arr[i] = callback_arr[i+1];
	}
	if (callback_amount < callback_total - 5 && callback_total - 5 > 0) {
		struct callback *tmp = realloc(callback_arr, sizeof(callback_total - 5));
		if (tmp == NULL) {
			printf("remove_callback: failed to reallocate callback_arr\n");
			return;
		}
		callback_arr = tmp;
		callback_total -= 5;
	}
	cb->is_qued = 0;
	callback_amount -= 1;
}

void remove_from_que(struct callback *cb)
{
	cb->times_called_back = 0;
	for (int i = cb->que_index; i < que_len - 1; i++) {
		callback_que[i] = callback_que[i+1];
		callback_que[i]->que_index = i;
	}
	if (que_len < que_total - 5 && que_total - 5 > 0) {
		struct callback **tmp = realloc(callback_que, sizeof(que_total - 5));
		if (tmp == NULL) {
			printf("remove_callback: failed to reallocate callback_arr\n");
			return;
		}
		callback_que = tmp;
		que_total -= 5;
	}
	cb->is_qued = 0;
	que_len -= 1;
}

void check_next_callback(void)
{
	if (next_cb == NULL) return;
	double delta = 0.0;
	next_cb->a = SDL_GetTicks();
	delta = next_cb->a - next_cb->b;
	if (delta > next_cb->timer) {
		next_cb->b = next_cb->a;
		next_cb->timer = 0.0;
		next_cb->function();
		if (next_cb->timer == 0.0) {
			remove_from_que(next_cb);
		}
		else 
			next_cb->times_called_back += 1;
		check_callbacks();
	}
}

void check_callbacks(void)
{
	int next_index = -1;
	double next_time = get_time_till_cb(callback_que[0]), delta = 0.0;
	for (int i = 0; i < que_len; i++) {
		delta = get_time_till_cb(callback_que[i]);
		if (delta <= next_time) {
			next_time = delta;
			next_index = i;
		}
	}
	if (next_index != -1) next_cb = callback_que[next_index];
	else next_cb = NULL;
}

int ui_init(void) 
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
	font = TTF_OpenFont("/usr/local/share/fonts/HackNerdFont-Regular.ttf", 20);
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
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_LEFT);

	next_cb = NULL;

	page_arr = malloc(sizeof(struct page *) * 5);
	page_arr_total_len = 5;
	page_arr_used_len = 0;
	showen_page = NULL;

	prompt_arr = malloc(sizeof(struct prompt *) * 1);
	prompt_arr_len = 0;
	showen_prompt = NULL;

	cursor = create_line(0, 0, 0, 0, WHITE, NULL);
	cursor->parent_p = NULL;
	cursor->show = 0;

	SDL_StartTextInput();
	return 0;
}

void ui_shutdown(void)
{
	SDL_StopTextInput();
	destroy_line(cursor);

	while (page_arr_used_len > 0) {
		destroy_page(page_arr[page_arr_used_len-1]);
	}
	while (prompt_arr_len > 0) {
		destroy_prompt(prompt_arr[prompt_arr_len-1]);
	}
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
	free(callback_arr);
	free(callback_que);
	free(page_arr);
	free(prompt_arr);
	TTF_Quit();
	SDL_Quit();
}

void show_screen(void)
{
	SDL_RenderPresent(renderer);
}

void clear_screen(color c)
{
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(c));
	SDL_RenderClear(renderer);
}

void check_events_and_callbacks(Event *event)
{
	check_next_callback();
	while (SDL_PollEvent(event)) {
		handle_event(event);
	}
}

void handle_event(Event *event)
{
	switch (event->type) {
		case SDL_QUIT:
			running = 0;
			break;
		case SDL_MOUSEBUTTONDOWN:
			//mabey create a mouse_button_down function for stuff that should happen regardless of left or right
			if (event->button.button == SDL_BUTTON_LEFT) lmouse_button_down(event);
			if (event->button.button == SDL_BUTTON_RIGHT) rmouse_button_down(event);
			break;
		case SDL_MOUSEBUTTONUP:
			if (event->button.button == SDL_BUTTON_LEFT) lmouse_button_up(event);
			if (event->button.button == SDL_BUTTON_RIGHT) rmouse_button_up(event);
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
					if (showen_page->selected_i != NULL || showen_prompt != NULL) {
						struct input *selected = showen_prompt == NULL ? showen_page->selected_i : showen_prompt->selected_input;
						on_backspace_input(selected);
					}
					break;
				default:
					break;
			}
			break;
		case SDL_TEXTINPUT:
			if ((showen_page->selected_i != NULL || showen_prompt != NULL)){
				struct input *selected = showen_prompt == NULL ? showen_page->selected_i : showen_prompt->selected_input;
				on_text_input(selected, &event->text);
			}
			break;
		default:
			break;
	}
}

static void on_backspace_input(struct input *selected)
{
	if (selected == NULL) return;
	size_t len = strlen(selected->text->str);
	char tmpstr[MAX_TEXT_SIZE];
	strcpy(tmpstr, selected->text->str);
	tmpstr[len-1] = '\0';
	change_input_box_text(selected, tmpstr);
}

static void on_text_input(struct input *selected, SDL_TextInputEvent *e)
{
	if (selected == NULL) return;
	size_t len = strlen(selected->text->str);
	if ((len >= selected->max_len && selected->max_len != 0) || len >= MAX_TEXT_SIZE-1) return;
	char tmpstr[MAX_TEXT_SIZE]; 
	if (selected->filter != NULL && selected->filter(selected, e->text) == 0) {
		strncpy(tmpstr, selected->text->str, MAX_TEXT_SIZE);
		strncat(tmpstr, e->text, MAX_TEXT_SIZE - len);
	} else if (selected->filter == NULL) {
		strncpy(tmpstr, selected->text->str, MAX_TEXT_SIZE);
		strncat(tmpstr, e->text, MAX_TEXT_SIZE - len);
	} else {
		strncpy(tmpstr, selected->text->str, MAX_TEXT_SIZE);
	}
	change_input_box_text(selected, tmpstr);
}

static void scroll_ddm(struct drop_down_menu *ddm, int wheely)
{
	int th = 0;
	for (int i = 0; i < ddm->items; i++) {
		th += ddm->text[i]->dst.h;
		if (th > ddm->drop_pos.h) break;
	}
	if (th < ddm->drop_pos.h) {
		ddm->scroll_offset = 0;
		return;
	}
	int lty = ddm->text[ddm->items-1]->dst.y; 
	int lth = ddm->text[ddm->items-1]->dst.h;
	int dpy = ddm->drop_pos.y, dph = ddm->drop_pos.h;

	ddm->scroll_offset += wheely * 10;
	ddm->update_highlight = 1;
	if (ddm->scroll_offset > 0) 
		ddm->scroll_offset = 0;
	else if (ddm->scroll_offset < -(lty + lth - dpy - dph)) 
		ddm->scroll_offset = -(lty + lth - dpy - dph);
}

static void mouse_wheel(SDL_MouseWheelEvent *event)
{
	int wheely = event->y;
	rect_s mouse_pos = { event->mouseX, event->mouseY, 0 };
	if (showen_page->selected_d != NULL && CHECK_RECT(mouse_pos, showen_page->selected_d->drop_pos) ) {
		scroll_ddm(showen_page->selected_d, wheely);
	}
}

static void mouse_move(Event *event)
{
	mouse_x = event->motion.x;
	mouse_y = event->motion.y;
	if (showen_page->selected_b != NULL && showen_page->selected_b->movable == 1) {
		showen_page->selected_b->on_move(showen_page->selected_b, event);
	}
	else if (showen_page->selected_b == NULL) {
		for (int i = 0; i < showen_page->b_arr_used_len; i++) {
			if (showen_page->b_arr[i]->hoverable != 0 && CHECK_RECT(event->motion, showen_page->b_arr[i]->pos)) {
				showen_page->b_arr[i]->hovering = 1;
				showen_page->b_arr[i]->on_hover(showen_page->b_arr[i], event);
			} else if (showen_page->b_arr[i]->hoverable != 0) {
				showen_page->b_arr[i]->hovering = 0;
				showen_page->b_arr[i]->on_hover(showen_page->b_arr[i], event);
			}
		}
	}
	if (showen_page->selected_d != NULL && CHECK_RECT(event->motion, showen_page->selected_d->drop_pos)){ 
		showen_page->selected_d->update_highlight = 1;
	}
	if (showen_page->selected_s != NULL) {
		update_slider(showen_page->selected_s, mouse_x);
		if (showen_page->selected_s->on_move) 
			showen_page->selected_s->on_move(showen_page->selected_s, event);
	}
	if (showen_page->selected_g != NULL) {
		if (showen_page->selected_g->selected_point != NULL && event->motion.state == SDL_PRESSED) {
			if (event->motion.x <= showen_page->selected_g->scaled_pos.x + showen_page->selected_g->scaled_pos.w 
					&& event->motion.x >= showen_page->selected_g->scaled_pos.x) {
				showen_page->selected_g->selected_point->x = (event->motion.x - showen_page->selected_g->scaled_pos.x)/showen_page->selected_g->scale_w;
			}

			if (event->motion.y <= showen_page->selected_g->scaled_pos.y + showen_page->selected_g->scaled_pos.h 
					&& event->motion.y >= showen_page->selected_g->scaled_pos.y) {
				showen_page->selected_g->selected_point->y = (event->motion.y - showen_page->selected_g->scaled_pos.y)/showen_page->selected_g->scale_h;
			}
		}
		if (showen_page->selected_g->on_move != NULL) showen_page->selected_g->on_move(showen_page->selected_g, event);
	}
}

static void rmouse_button_down(Event *event)
{
	
}

static void rmouse_button_up(Event *event)
{
	SDL_MouseButtonEvent mouse_data = event->button;
	for (int i = 0; i < showen_page->g_arr_used_len; i++) {
		if (CHECK_RECT(mouse_data, showen_page->g_arr[i]->scaled_pos)) {
			showen_page->selected_g = showen_page->g_arr[i];
			int offx = mouse_data.x - showen_page->selected_g->scaled_pos.x;
			int offy = mouse_data.y - showen_page->selected_g->scaled_pos.y;
			for (int j = 0; j < showen_page->selected_g->point_amount; j++) {
				struct graph *tmp_graph = showen_page->selected_g;
				if (offx < tmp_graph->points[j].x * tmp_graph->scale_w + tmp_graph->points_size.w && offx > tmp_graph->points[j].x * tmp_graph->scale_w
						&& offy < tmp_graph->points[j].y * tmp_graph->scale_h + tmp_graph->points_size.h && offy > tmp_graph->points[j].y * tmp_graph->scale_h) {
					tmp_graph->point_amount -= 1;
					for (int i = j; i < tmp_graph->point_amount; i++) {
						tmp_graph->points[i] = tmp_graph->points[i + 1];
					}
					tmp_graph->points[tmp_graph->point_amount].x = 0;
					tmp_graph->points[tmp_graph->point_amount].y = 0;
					tmp_graph->selected_point = NULL;
				}
			}
		}
	}
}

static void lmbu_prompt(Event *e)
{
	SDL_MouseButtonEvent mouse_data = e->button;
	if (showen_prompt->selected_button != NULL) {
		struct button *tmp_self = NULL;
		if (showen_prompt->selected_button->clickable) {
			tmp_self = showen_prompt->selected_button;
		}
		showen_prompt->selected_button = NULL;
		if (tmp_self != NULL) tmp_self->on_click(tmp_self, e);
		return;
	}
	int hit = 0;
	for (int i = 0; i < showen_prompt->input_arr_used; i++) {
		if (CHECK_RECT(mouse_data, showen_prompt->input_arr[i]->pos))  {
			hit = 1;
			showen_prompt->input_arr[i]->selected = 1;
			showen_prompt->selected_input = showen_prompt->input_arr[i];
			cursor->show = 1;
			int text_str_len = strlen(showen_prompt->input_arr[i]->text->str);
			cursor->start.x = showen_prompt->input_arr[i]->text->dst.x + showen_prompt->input_arr[i]->char_size.w * text_str_len;
			cursor->start.y = showen_prompt->input_arr[i]->text->dst.y;
			cursor->too.x = showen_prompt->input_arr[i]->text->dst.x + showen_prompt->input_arr[i]->char_size.w * text_str_len;
			cursor->too.y = showen_prompt->input_arr[i]->text->dst.y + showen_prompt->input_arr[i]->char_size.h;
			SDL_StartTextInput();
		} else if (showen_prompt->input_arr[i]->selected != 0) {
			showen_prompt->input_arr[i]->selected = 0;
		}
	}
	if (hit == 0) {
		SDL_StopTextInput();
		showen_prompt->selected_input = NULL;
		cursor->show = 0;
	}

}

/* LATER: only check if button or input_box is visible mabey create array with all showen things idk */
static void lmouse_button_up(Event *event)
{
	SDL_MouseButtonEvent mouse_data = event->button;
	if (showen_prompt != NULL) {
		lmbu_prompt(event);
		return;
	}
	if (showen_page->selected_b != NULL && showen_page->selected_b->clickable == 1) {
		showen_page->selected_b->on_click(showen_page->selected_b, event);
		showen_page->selected_b = NULL;
	} 
	if (showen_page->selected_s != NULL) {
		if (showen_page->selected_s->button->on_click != NULL) showen_page->selected_s->button->on_click(showen_page->selected_s->button, event);
		showen_page->selected_s = NULL;

	}
	if (showen_page->selected_g != NULL) {
		if (event->button.clicks > 1) {
			if (showen_page->selected_g->total_points < showen_page->selected_g->point_amount + 1) {
				showen_page->selected_g->total_points += 5;
				showen_page->selected_g->points = realloc(showen_page->selected_g->points, sizeof(struct point) * showen_page->selected_g->total_points);
			}
			struct graph *tmp_g = showen_page->selected_g;
			for (int i = 0; i < tmp_g->point_amount + 1; i++) {
				if (tmp_g->points[i].x < (mouse_data.x-tmp_g->real_pos.x)/tmp_g->scale_w 
						&& tmp_g->points[i + 1].x > (mouse_data.x - tmp_g->real_pos.x)/tmp_g->scale_w) {
					struct point tmp_point = tmp_g->points[i + 1];
					for (int j = i + 1; j < tmp_g->point_amount; j++) {
						struct point other_tmp_point = tmp_g->points[j + 1];
						tmp_g->points[j + 1] = tmp_point;
						tmp_point = other_tmp_point;
						
					}
					tmp_g->points[i + 1].x = (mouse_data.x - tmp_g->real_pos.x)/tmp_g->scale_w;
					tmp_g->points[i + 1].y = (mouse_data.y - tmp_g->real_pos.y)/tmp_g->scale_h;
					tmp_g->point_amount++;
					break;
				} else if (i == tmp_g->point_amount && tmp_g->points[i-1].x < (mouse_data.x-tmp_g->real_pos.x)/tmp_g->scale_w) {
					tmp_g->points[i].x = (mouse_data.x - tmp_g->real_pos.x)/tmp_g->scale_w;
					tmp_g->points[i].y = (mouse_data.y - tmp_g->real_pos.y)/tmp_g->scale_h;
					tmp_g->point_amount++;
					break;
				} else if (i == 0 && tmp_g->points[i].x > (mouse_data.x-tmp_g->real_pos.x)/tmp_g->scale_w) {
					struct point tmp_point = tmp_g->points[i];
					for (int j = i + 1; j < tmp_g->point_amount + 1; j++) {
						struct point other_tmp_point = tmp_g->points[j];
						tmp_g->points[j] = tmp_point;
						tmp_point = other_tmp_point;
						
					}
					tmp_g->points[i].x = (mouse_data.x - tmp_g->real_pos.x)/tmp_g->scale_w;
					tmp_g->points[i].y = (mouse_data.y - tmp_g->real_pos.y)/tmp_g->scale_h;
					tmp_g->point_amount++;
					break;
				}
			}
		} else {
			showen_page->selected_g = NULL;
		}
	}
	int hit = 0;
	for (int i = 0; i < showen_page->i_arr_used_len; i++) {
		if (CHECK_RECT(mouse_data, showen_page->i_arr[i]->pos))  {
			hit = 1;
			showen_page->i_arr[i]->selected = 1;
			showen_page->selected_i = showen_page->i_arr[i];
			cursor->show = 1;
			int text_str_len = strlen(showen_page->i_arr[i]->text->str);
			cursor->start.x = showen_page->i_arr[i]->text->dst.x + showen_page->i_arr[i]->char_size.w * text_str_len;
			cursor->start.y = showen_page->i_arr[i]->text->dst.y;
			cursor->too.x = showen_page->i_arr[i]->text->dst.x + showen_page->i_arr[i]->char_size.w * text_str_len;
			cursor->too.y = showen_page->i_arr[i]->text->dst.y + showen_page->i_arr[i]->char_size.h;
			SDL_StartTextInput();
		} else if (showen_page->i_arr[i]->selected != 0) {
			showen_page->i_arr[i]->selected = 0;
		}
	}
	if (hit == 0) {
		SDL_StopTextInput();
		showen_page->selected_i = NULL;
		cursor->show = 0;
	}
	hit = 0;
	if (showen_page->selected_d != NULL && CHECK_RECT(mouse_data, showen_page->selected_d->drop_pos)) {
		for (int i = 0; i < showen_page->selected_d->items; i++) {
			rect_s pos = showen_page->selected_d->text[i]->dst;
			if (showen_page->selected_d->text[i]->show && mouse_data.x < pos.x + showen_page->selected_d->default_pos.w && mouse_data.x > pos.x 
					&& mouse_data.y < pos.y + pos.h + showen_page->selected_d->scroll_offset && mouse_data.y > pos.y + showen_page->selected_d->scroll_offset) {
				showen_page->selected_d->selected_text_index = i;
				showen_page->selected_d->text[i]->show = 1;
				showen_page->selected_d->scroll_offset = -(showen_page->selected_d->text[i]->dst.y - showen_page->selected_d->used_pos.y - 2);
				if (showen_page->selected_d->function != NULL) showen_page->selected_d->function(showen_page->selected_d, event);
				showen_page->selected_d->selected = 0;
				showen_page->selected_d = NULL;
				break;
			}
		}
	} else {
		for (int i = 0; i < showen_page->d_arr_used_len; i++) {
			if (CHECK_RECT(mouse_data, showen_page->d_arr[i]->used_pos)) {
				hit = 1;
				showen_page->d_arr[i]->selected = 1;
				showen_page->selected_d = showen_page->d_arr[i];
			} else if (showen_page->d_arr[i]->selected != 0) {
				showen_page->d_arr[i]->selected = 0;
			}
		}
		if (hit == 0) {
			if (showen_page->selected_d != NULL) 
				showen_page->selected_d->scroll_offset = -(showen_page->selected_d->text[showen_page->selected_d->selected_text_index]->dst.y - showen_page->selected_d->used_pos.y - 2);
			showen_page->selected_d = NULL;
		}
	}
}


static void lmbd_prompt(Event *e)
{
	for (int i = 0; i < showen_prompt->button_arr_used; i++) {
		if (showen_prompt->button_arr[i]->clickable == 1 && CHECK_RECT(e->button, showen_prompt->button_arr[i]->pos)) {
			showen_prompt->selected_button = showen_prompt->button_arr[i];
		}
	}
}

static void lmouse_button_down(Event *event)
{
	SDL_MouseButtonEvent mouse_data = event->button;
	if (showen_prompt != NULL) {
		lmbd_prompt(event);
		return;
	}
	for (int i = 0; i < showen_page->b_arr_used_len; i++) {
		if (showen_page->b_arr[i]->clickable == 1 && CHECK_RECT(event->button, showen_page->b_arr[i]->pos)) {
			showen_page->selected_b = showen_page->b_arr[i];
		}
	}
	for (int i = 0; i < showen_page->s_arr_used_len; i++) {
		if (CHECK_RECT(mouse_data, showen_page->s_arr[i]->button->pos)) {
			showen_page->selected_s = showen_page->s_arr[i];
		}
	}
	if (showen_page->selected_g == NULL) {
		for (int i = 0; i < showen_page->g_arr_used_len; i++) {
			if (CHECK_RECT(mouse_data, showen_page->g_arr[i]->scaled_pos) && showen_page->g_arr[i]->show != 0) {
				struct graph *tmp_g = showen_page->selected_g = showen_page->g_arr[i];
				int offx = (mouse_data.x - tmp_g->scaled_pos.x);
				int offy = (mouse_data.y - tmp_g->scaled_pos.y);
				int h = 0;
				for (int j = 0; j < tmp_g->point_amount; j++) {
					if (tmp_g->points[j].x * tmp_g->scale_w + tmp_g->points_size.w > offx && tmp_g->points[j].x * tmp_g->scale_w < offx && 
							tmp_g->points[j].y * tmp_g->scale_h + tmp_g->points_size.h > offy && tmp_g->points[j].y * tmp_g->scale_h < offy) {
						tmp_g->selected_point = &tmp_g->points[j];
						tmp_g->selected_point_index = j;
						tmp_g->selected = 1;
						h = 1;
					} else if (offx > tmp_g->scaled_pos.w - tmp_g->points_size.w 
						 && offx + tmp_g->points_size.w < tmp_g->points[j].x * tmp_g->scale_w + tmp_g->points_size.h && offx + tmp_g->points_size.w > tmp_g->points[j].x * tmp_g->scale_w
						 && offy < tmp_g->points[j].y * tmp_g->scale_h + tmp_g->points_size.h && offy > tmp_g->points[j].y * tmp_g->scale_h) {
						tmp_g->selected_point = &tmp_g->points[j];
						tmp_g->selected_point_index = j;
						tmp_g->selected = 1;
						h = 1;
					} else if (offy > tmp_g->scaled_pos.h - tmp_g->points_size.h 
						 && offx < tmp_g->points[j].x * tmp_g->scale_w + tmp_g->points_size.h && offx > tmp_g->points[j].x * tmp_g->scale_w
						 && offy + tmp_g->points_size.h < tmp_g->points[j].y * tmp_g->scale_h + tmp_g->points_size.h && offy + tmp_g->points_size.h > tmp_g->points[j].y * tmp_g->scale_h) {
						tmp_g->selected_point = &tmp_g->points[j];
						tmp_g->selected_point_index = j;
						tmp_g->selected = 1;
						h = 1;
					} 
				} 
				if (h == 0) tmp_g->selected_point = NULL;
				else if (tmp_g->on_click != NULL) tmp_g->on_click(tmp_g, event);
			}
		}
	}
}

texture_s *create_texture_from_surface(surface_s *sur)
{
	texture_s *textest = SDL_CreateTextureFromSurface(renderer, sur);
	if (textest == NULL) {
		printf("create texture did a epic fail\n");
		printf("%s\n", SDL_GetError());
		return NULL;
	}
	return textest;
}

struct page *create_page(void)
{
	struct page *return_page = malloc(sizeof(struct page));
	return_page->t_arr = malloc(sizeof(struct text *) * 40);
	return_page->t_arr_total_len = 10;
	return_page->t_arr_used_len = 0;

	return_page->i_arr = malloc(sizeof(struct input*) * 10);
	return_page->i_arr_total_len = 10;
	return_page->i_arr_used_len = 0;
	return_page->selected_i = NULL;

	return_page->b_arr = malloc(sizeof(struct button*) * 10);
	return_page->b_arr_total_len = 10;
	return_page->b_arr_used_len = 0;
	return_page->selected_b = NULL;

	return_page->d_arr = malloc(sizeof(struct drop_down_menu*) * 10);
	return_page->d_arr_total_len = 10;
	return_page->d_arr_used_len = 0;
	return_page->selected_d = NULL;

	return_page->g_arr = malloc(sizeof(struct graph*) * 10);
	return_page->g_arr_total_len = 10;
	return_page->g_arr_used_len = 0;
	return_page->selected_g = NULL;

	return_page->s_arr = malloc(sizeof(struct slider*) * 10);
	return_page->s_arr_total_len = 10;
	return_page->s_arr_used_len = 0;
	return_page->selected_s = NULL;

	return_page->img_arr = malloc(sizeof(struct image*) * 5);
	return_page->img_arr_total_len = 5;
	return_page->img_arr_used_len = 0;

	return_page->line_arr = malloc(sizeof(struct line*) * 5);
	return_page->line_arr_total_len = 5;
	return_page->line_arr_used_len = 0;

	if (page_arr_used_len + 1 > page_arr_total_len) {
		page_arr = realloc(page_arr, sizeof(struct page *) * (page_arr_total_len + 1));
		page_arr_total_len += 1;
	}
	return_page->index = page_arr_used_len;

	page_arr[page_arr_used_len] = return_page;
	page_arr_used_len += 1;

	return return_page;
}

void destroy_page(struct page *page)
{
	/*
	struct input **tmp_iarr = page->i_arr;
	struct button **tmp_barr = page->b_arr;
	struct drop_down_menu **tmp_darr = page->d_arr;
	struct slider **tmp_sarr = page->s_arr;
	struct text **tmp_tarr = page->t_arr;
	struct graph **tmp_garr = page->g_arr;
	*/
	for (int i = 0; i < page->img_arr_used_len; i++) {
		destroy_image(page->img_arr[page->img_arr_used_len-1]);
	}
	while (page->i_arr_used_len > 0) {
		destroy_input_box(page->i_arr[page->i_arr_used_len-1]);
	}
	while (page->b_arr_used_len > 0) {
		destroy_button(page->b_arr[page->b_arr_used_len-1]);
	}
	while (page->d_arr_used_len > 0) {
		destroy_ddm(page->d_arr[page->d_arr_used_len-1]);
	}
	while (page->s_arr_used_len > 0) {
		destroy_slider(page->s_arr[page->s_arr_used_len-1]);
	}
	while (page->t_arr_used_len > 0) {
		destroy_text(page->t_arr[page->t_arr_used_len-1]);
	}
	while (page->g_arr_used_len > 0) {
		destroy_graph(page->g_arr[page->g_arr_used_len-1]);
	}
	while (page->line_arr_used_len > 0) {
		destroy_line(page->line_arr[page->line_arr_used_len-1]);
	}
	for (int i = page->index; i < page_arr_used_len - 1; i++) {
		page_arr[i] = page_arr[i + 1];
		page_arr[i]->index = i;
	}
	page_arr_used_len -= 1;
	free(page->i_arr);
	free(page->b_arr);
	free(page->s_arr);
	free(page->t_arr);
	free(page->g_arr);
	free(page->line_arr);
	free(page->img_arr);
	free(page);
}
void show_page(struct page *page)
{
	if (showen_page != NULL)
		showen_page->show = 0;

	page->show = 1;
	showen_page = page;
}

void render_showen_page(void)
{
	if (showen_page == NULL) return;
	for (int i = 0; i < showen_page->img_arr_used_len; i++) {
		show_image(showen_page->img_arr[i]);
	}
	for (int i = 0; i < showen_page->b_arr_used_len; i++) {
		if (showen_page->b_arr[i]->show != 0)render_button(showen_page->b_arr[i]);
	}
	for (int i = 0; i < showen_page->t_arr_used_len; i++) {
		if (showen_page->t_arr[i]->show != 0)render_text(showen_page->t_arr[i], NULL);
	}
	for (int i = 0; i < showen_page->g_arr_used_len; i++) {
		if (showen_page->g_arr[i]->show != 0) render_graph(showen_page->g_arr[i]);
	}
	for (int i = 0; i < showen_page->i_arr_used_len; i++) {
		if (showen_page->i_arr[i]->show != 0) render_input_box(showen_page->i_arr[i]);
	}
	for (int i = 0; i < showen_page->s_arr_used_len; i++) {
		if (showen_page->s_arr[i]->show != 0) render_slider(showen_page->s_arr[i]);
	}
	for (int i = 0; i < showen_page->d_arr_used_len; i++) {
		if (showen_page->d_arr[i]->show != 0 && showen_page->selected_d != showen_page->d_arr[i]) 
			render_ddm(showen_page->d_arr[i]);
	}
	for (int i = 0; i < showen_page->line_arr_used_len; i++) {
		if (showen_page->line_arr[i]->show != 0) render_line(showen_page->line_arr[i]);
	}
	if (showen_page->selected_d != NULL) {
		render_ddm(showen_page->selected_d);
	}
	if (showen_prompt != NULL) {
		render_prompt(showen_prompt);
	}
	if (cursor->show != 0) render_line(cursor);
}

struct text *create_text(char *string, int x, int y, int w, int h, int font_size, int wrap_length, color  fg_color, TTF_Font *f, struct page *p)
{
	struct text *return_text = malloc(sizeof(struct text));
	struct text new_text = { "", 1, 0, 0, 0, wrap_length, fg_color, { 0 }, { x, y, 0, 0 }, NULL, 0, p };
	
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
	if (render_text_texture(&new_text, new_text.fg_color, f) != 0) {
		printf("ui.c render_text_texture failed from create_text\n");
	}
	memcpy(return_text, &new_text, sizeof(struct text));
	if (p != NULL) {
		if (p->t_arr_used_len + 1 > p->t_arr_total_len) {
			p->t_arr = realloc(p->t_arr, sizeof(struct text *) * (p->t_arr_total_len + 10));
			p->t_arr_total_len += 10;
		}
		return_text->index = p->t_arr_used_len;
		p->t_arr[p->t_arr_used_len] = return_text;
		p->t_arr_used_len += 1;
	}

	return return_text;
}

void destroy_text(struct text *text)
{
	//printf("destroy text start\ntext->str = %s\ntext->parent_p = %p\n", text->str, (void *)text->parent_p);
	if (text == NULL) return;
	//strcpy(text->str, "good");
	destroy_text_texture(text);
	//printf("destroy text %s\n", text->str);
	if (text->parent_p != NULL) {
		//printf("change_parent arr\n");
		for (int i = text->index; i < text->parent_p->t_arr_used_len-1; i++) {
			text->parent_p->t_arr[i] = text->parent_p->t_arr[i+1];
			text->parent_p->t_arr[i]->index = i;
		}
		text->parent_p->t_arr[text->parent_p->t_arr_used_len] = NULL;
		text->parent_p->t_arr_used_len -= 1;
	}
	//printf("free text start, text->str = %s, text %p\n", text->str, (void *)text);
	free(text);
	//printf("free text done, errno = %d\n", errno);
}

void destroy_text_texture(struct text *text)
{
#ifdef INFO
	printf("destroy text texture:\n");
	printf("text->str = \"%s\", ", text->str);
	printf("text->index = %d\n", text->index);
#endif
	SDL_DestroyTexture(text->texture);
}

int prev_font_size = 20;
int render_text_texture(struct text *t, color fg_color, TTF_Font *f)
{
	SDL_Color cast_color = *((SDL_Color*) &fg_color);
	if (prev_font_size != t->font_size) {
		TTF_SetFontSize(f, t->font_size);
		prev_font_size = t->font_size;
	}
	surface_s *surface = TTF_RenderText_Blended_Wrapped(f, t->str, cast_color, t->wrap_length);
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
	t->fg_color = fg_color;
	SDL_FreeSurface(surface);
	return 0;
}

void change_text_and_render_texture(struct text *text, char *new_text, color  fg_color, TTF_Font *f)
{
	if (strlen(new_text) < MAX_TEXT_SIZE) {
		strcpy(text->str, new_text);
		render_text_texture(text, fg_color, f);
	}
}

void render_text(struct text *t, rect_s *src)
{
	if (t->show != 0) 
		SDL_RenderCopy(renderer, t->texture, src, &t->dst);
}

struct button *create_button(char *string, int movable, int show, int x, int y, int w, int h, int font_size, TTF_Font *f, void (*on_click)(), void (*on_move)(), color  outer_color, color  bg_color, color  text_color, struct page *p)
{
	struct button *return_button = malloc(sizeof(struct button));
	struct button new_button = { NULL, NULL, on_click, on_move, NULL, 0, movable, 0, 0, show, 0, { x, y, }, outer_color, bg_color, p };

	if (on_click != NULL) new_button.clickable = 1;
	if (string != NULL) {
		int tmp_w = 0, tmp_h = 0;
		if (h != 0) tmp_h = h - 10;
		if (w != 0) tmp_w = w - 10;
		new_button.text = create_text(string, x + 5, y + 5, tmp_w, tmp_h, font_size, 0, text_color, f, NULL);
		new_button.texture = &new_button.text->texture;
	} 
	new_button.pos.w = w == 0 ? new_button.text->dst.w + 10 : w;
	new_button.pos.h = h == 0 ? new_button.text->dst.h + 10 : h;
	memcpy(return_button, &new_button, sizeof(struct button));
	if (p != NULL) {
		if (p->b_arr_used_len + 1 > p->b_arr_total_len) {
			p->b_arr = (struct button**)realloc(p->b_arr, sizeof(struct button*) * (p->b_arr_total_len + 5));
			p->b_arr_total_len += 5;
		}
		return_button->index = p->b_arr_used_len;
		p->b_arr[p->b_arr_used_len] = return_button;
		p->b_arr_used_len += 1;
	}

	return return_button;
}

void destroy_button(struct button *button)
{
	PRINTINFO_VA("destroying button:\nindex = %d, str = %s\nparrent addr = %p\n", button->index,  button->text->str, (void *)button->parent_p);
	if (button->text != NULL) destroy_text(button->text);
	PRINTINFO("destroy text done\n");

	if (button->parent_p != NULL) {
		for (int i = button->index; i < button->parent_p->b_arr_used_len-1; i++) {
			button->parent_p->b_arr[i] = button->parent_p->b_arr[i+1];
			button->parent_p->b_arr[i]->index = i;
		}
		button->parent_p->b_arr_used_len -= 1;
	}
	PRINTINFO_VA("start free, button at %p, index = %d\n", (void *)button, button->index);
	free(button);
	PRINTINFO("done free\n");
}
void render_button(struct button *button)
{
	if (button == NULL) {
		printf("error passed a NULL pointer to render_button\n");
		return;
	}
	if (button->show == 0 ) return;
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(button->bg_color));
	SDL_RenderFillRect(renderer, &button->pos);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(button->outer_color));
	SDL_RenderDrawRect(renderer, &button->pos);
	if (button->text != NULL) render_text(button->text, NULL);
}

// 	   |---------|
// 	-> |some text|
// 	   |---------|
struct input *create_input(char *string, char *def_text, int resize_box, int max_len, int x, int y, int w, int h, void (*function)(), TTF_Font *f, color  outer_color, color  bg_color, color  text_color, struct page *p)
{
	struct input *return_input = malloc(sizeof(struct input));
	struct input new_input = { 0, 0, max_len, 1, NULL, NULL, resize_box, function, NULL, { x, y, w, h }, {x, y, w, h }, outer_color, bg_color, { 0 }, p };

	if (string != NULL) {
		if (max_len != 0) {
			char tstr[max_len+1];
			for (int i = 0; i < max_len; i++) {
				tstr[i] = ' ';
			}
			tstr[max_len] = '\0';
			new_input.text = create_text(tstr, x+5, y+5, 0, 0, 20, 0, text_color, f, NULL);
			w = new_input.text->dst.w + 10;
			h = new_input.text->dst.h + 10;
			tstr[1] = '\0';
			change_text_and_render_texture(new_input.text, tstr, text_color, f);
			new_input.char_size.w = new_input.text->dst.w;
			new_input.char_size.h = new_input.text->dst.h;
			change_text_and_render_texture(new_input.text, string, text_color, f);
		}
		else {
			char tstr[2];
			for (int i = 0; i < 2; i++) {
				tstr[i] = ' ';
			}
			tstr[1] = '\0';
			new_input.text = create_text(tstr, x+5, y+5, 0, 0, 20, 0, text_color, f, NULL);
			new_input.char_size.w = new_input.text->dst.w;
			new_input.char_size.h = new_input.text->dst.h;
			change_text_and_render_texture(new_input.text, string, text_color, f);
		}
	}
	if (def_text != NULL) {
		new_input.default_text = create_text(def_text, x+5, y+5, 0, 0, 20, 0, text_color, f, NULL);
	}

	if (w == 0 || resize_box) {
		new_input.default_pos.w = new_input.pos.w = new_input.text->dst.w + 10;
	} else new_input.default_pos.w = new_input.pos.w = w;
	if (h == 0 || resize_box) {
		new_input.default_pos.h = new_input.pos.h = new_input.text->dst.h + 10;
	} else new_input.default_pos.h = new_input.pos.h = h;

	if (!resize_box) {
		new_input.text->src = new_input.text->dst;
		new_input.text->src.x = 0;
		new_input.text->src.y = 0;
	}
	new_input.default_text->src = new_input.text->src;
	if (new_input.text->str[0] == '\0') {
		new_input.default_text->show = 1;
		new_input.text->show = 0;
	}

	memcpy(return_input, &new_input, sizeof(struct input));
	if (p != NULL) {
		if (p->i_arr_used_len + 1 > p->i_arr_total_len) {
			p->i_arr = (struct input**)realloc(p->i_arr, sizeof(struct input*) * (p->i_arr_total_len + 5));
			p->i_arr_total_len += 5;
		}
		return_input->index = p->i_arr_used_len;
		p->i_arr[p->i_arr_used_len] = return_input;
		p->i_arr_used_len += 1;
	}

	return return_input;
}

void change_input_box_text(struct input *input_box, char *str)
{
	rect_s prev_input_margin = input_box->pos;
	prev_input_margin.w = input_box->pos.w - input_box->text->dst.w;
	prev_input_margin.h = input_box->pos.h - input_box->text->dst.h;

	size_t old_len = strlen(input_box->text->str), new_len = strlen(str);
	strncpy(input_box->text->str, str, MAX_TEXT_SIZE);
	if (str[0] != '\0') {
		render_text_texture(input_box->text, input_box->text->fg_color, font);
		input_box->text->show = 1;
		input_box->default_text->show = 0;
	} else {
		input_box->default_text->dst.x = input_box->text->dst.x;
		input_box->default_text->dst.y = input_box->text->dst.y;
		render_text_texture(input_box->default_text, input_box->default_text->fg_color, font);
		input_box->text->show = 0;
		input_box->default_text->show = 1;
	}

	if (input_box->resize_box == 1) {
		input_box->text->src.x = 0;
		input_box->text->src.y = 0;
		input_box->text->src.w = input_box->text->dst.w;
		input_box->text->src.h = input_box->text->dst.h;
		input_box->pos.w = input_box->text->dst.w + prev_input_margin.w;
		input_box->pos.h = input_box->text->dst.h + prev_input_margin.h;
	} else {
		if (old_len < new_len && input_box->pos.w < input_box->text->dst.w) {
			input_box->text->src.x += input_box->char_size.w;
			input_box->text->src.w = input_box->text->dst.w = input_box->default_pos.w - prev_input_margin.w;
			cursor->too.x = cursor->start.x = input_box->text->dst.x + input_box->text->dst.w - 2;
		} else if (old_len > new_len && input_box->pos.w < input_box->text->dst.w) {
			input_box->text->src.x -= input_box->char_size.w;
			input_box->text->dst.w = input_box->text->src.w = input_box->default_pos.w - prev_input_margin.w;
			cursor->too.x = cursor->start.x = input_box->text->dst.x + input_box->text->dst.w - 2;
		} else {
			input_box->text->src.y = input_box->text->src.x = 0;
			input_box->text->src.w = input_box->pos.w = input_box->default_pos.w;
			input_box->text->src.h = input_box->pos.h = input_box->default_pos.h;
			if (input_box->default_text->show) cursor->too.x = cursor->start.x = input_box->text->dst.x;
			else cursor->too.x = cursor->start.x = input_box->text->dst.x + input_box->text->dst.w;
		}
	}
}

void render_input_box(struct input *input_box)
{
	if (input_box == NULL) {
		printf("error: passed a NULL pointer to render_input_box");
		return;
	}
	if (input_box->show == 0) return;
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(input_box->bg_color));
	SDL_RenderFillRect(renderer, &input_box->pos);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(input_box->outer_color));
	SDL_RenderDrawRect(renderer, &input_box->pos);
	if (input_box->text->show) render_text(input_box->text, &input_box->text->src);
	else render_text(input_box->default_text, NULL);
}

void destroy_input_box(struct input *input_box)
{
	destroy_text(input_box->text);
	destroy_text(input_box->default_text);
	if (input_box->parent_p != NULL) {
		for (int i = input_box->index; i < input_box->parent_p->i_arr_used_len-1; i++) {
			input_box->parent_p->i_arr[i] = input_box->parent_p->i_arr[i+1];
			input_box->parent_p->i_arr[i]->index = i;
		}
		input_box->parent_p->i_arr[input_box->parent_p->i_arr_used_len] = NULL;
		input_box->parent_p->i_arr_used_len -= 1;
	}
	free(input_box);
}

struct drop_down_menu *create_drop_down_menu(int items, char item_str[][MAX_TEXT_SIZE], int x, int y, int w, int h, int dw, int dh, void (*function)(), TTF_Font *f, color  outer_color, color  bg_color, color  tc, struct page *p)
{
	rect_s pos = { x, y, w, h };
	rect_s dpos = { x, y, dw, dh };

	struct text **text_arr = malloc(sizeof(struct text *) * items);
	struct drop_down_menu *ddm_heap = malloc(sizeof(struct drop_down_menu));
	struct drop_down_menu ddm = { 0, 0, 0, 1, 0, 1, 1, items, text_arr, function, 0, pos, pos, dpos, {0}, outer_color, bg_color, p };
	if (pos.h == 0)
		ddm.static_h = 0;

	if (pos.w == 0)
		ddm.static_w = 0;
	int prev_height = 0;
	int bigest_w = ddm.default_pos.w, bigest_h = ddm.default_pos.h;
	for (int i = 0; i < items; i++) {
		ddm.text[i] = create_text(item_str[i], 0, 0, 0, 0, 0, w, tc, f, NULL);
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

	PRINTINFO_VA("create dmm\nddm.default_pos = { %d, %d, %d, %d }, ddm.used_pos = { %d, %d, %d, %d }, ddm.drop_pos = { %d, %d, %d, %d }\n", 
			ddm.default_pos.x, ddm.default_pos.y, ddm.default_pos.w, ddm.default_pos.h, ddm.used_pos.x, ddm.used_pos.y, ddm.used_pos.w, ddm.used_pos.h,
			ddm.drop_pos.x, ddm.drop_pos.y, ddm.drop_pos.w, ddm.drop_pos.h);

	memcpy(ddm_heap, &ddm, sizeof(struct drop_down_menu));
	if (p != NULL) {
		if (p->d_arr_used_len > p->d_arr_total_len) {
			struct drop_down_menu **tmp_ddm_arr = realloc(p->d_arr, sizeof(struct drop_down_menu*) * (p->d_arr_total_len + 5));
			if (tmp_ddm_arr != NULL) p->d_arr = tmp_ddm_arr;
			p->d_arr_total_len += 5;
		}
		ddm_heap->index = p->d_arr_used_len;
		p->d_arr[p->d_arr_used_len] = ddm_heap;
		p->d_arr_used_len++;
	}
	return ddm_heap;
}

void change_ddm_text_arr(struct drop_down_menu *ddm, int items, char newstr[][MAX_TEXT_SIZE], TTF_Font *f)
{
	for (int i = 0; i < ddm->items; i++) {
		destroy_text(ddm->text[i]);
	}
	ddm->text = realloc(ddm->text, sizeof(struct text *) * items);

	color  tc = ddm->text[0]->fg_color;
	int prev_height = 0;
	int bigest_w = ddm->default_pos.w, bigest_h = ddm->default_pos.h;
	for (int i = 0; i < items; i++) {
		ddm->text[i] = create_text(newstr[i], 0, 0, 0, 0, 0, ddm->default_pos.w, tc, f, NULL);
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
	PRINTINFO_VA("create dmm\nddm.default_pos = { %d, %d, %d, %d }, ddm.used_pos = { %d, %d, %d, %d }, ddm.drop_pos = { %d, %d, %d, %d }\n", 
			ddm->default_pos.x, ddm->default_pos.y, ddm->default_pos.w, ddm->default_pos.h, ddm->used_pos.x, ddm->used_pos.y, ddm->used_pos.w, ddm->used_pos.h,
			ddm->drop_pos.x, ddm->drop_pos.y, ddm->drop_pos.w, ddm->drop_pos.h);
			
}

void destroy_ddm(struct drop_down_menu *ddm) 
{
	for (int i = 0; i < ddm->items; i++) {
		destroy_text(ddm->text[i]);
	}
	if (ddm->parent_p != NULL) {
		for (int i = ddm->index; i < ddm->parent_p->d_arr_used_len-1; i++) {
			ddm->parent_p->d_arr[i] = ddm->parent_p->d_arr[i+1];
			ddm->parent_p->d_arr[i]->index = i;
		}
		ddm->parent_p->d_arr[ddm->parent_p->d_arr_used_len] = NULL;
		ddm->parent_p->d_arr_used_len -= 1;
	}
	free(ddm);
}

void select_ddm_item(struct drop_down_menu *ddm, int item)
{
	ddm->text[ddm->selected_text_index]->show = 0;
	ddm->selected_text_index = item;
	ddm->text[item]->show = 1;
	ddm->scroll_offset = -(ddm->text[item]->dst.y - ddm->used_pos.y - 2);
}

void add_item_ddm(struct drop_down_menu *ddm, char *newstr, TTF_Font *f) 
{
	ddm->text = realloc(ddm->text, sizeof(struct text *) * (++ddm->items));

	color  tc = ddm->text[0]->fg_color;
	int prev_height = 0;
	int bigest_w = ddm->default_pos.w, bigest_h = ddm->default_pos.h;
	ddm->text[ddm->items-1] = create_text(newstr, 0, 0, 0, 0, 0, ddm->default_pos.w, tc, f, NULL);
	for (int i = 0; i < ddm->items; i++) {
		if (!ddm->static_w && bigest_w < ddm->text[i]->dst.w) bigest_w = ddm->text[i]->dst.w;
		if (!ddm->static_h && bigest_h < ddm->text[i]->dst.h) bigest_h = ddm->text[i]->dst.h;
		ddm->text[i]->dst.y = ddm->default_pos.y + prev_height + 2;
		ddm->text[i]->dst.x = ddm->default_pos.x + 4;
		prev_height += ddm->text[i]->dst.h;
		ddm->text[i]->show = 0;
	}
	ddm->selected_text_index = 0;
	ddm->scroll_offset = 0;
	ddm->text[0]->show = 1;
}

void remove_item_ddm(struct drop_down_menu *ddm, int item)
{
	if (ddm->items <= 1) return;
	destroy_text(ddm->text[item]);
	for (int i = item; i < ddm->items-1; i++) {
		ddm->text[i] = ddm->text[i+1];
		ddm->text[i]->index = i;
	}
	int prev_height = 0;
	int bigest_w = ddm->default_pos.w, bigest_h = ddm->default_pos.h;
	for (int i = 0; i < ddm->items-1; i++) {
		if (!ddm->static_w && bigest_w < ddm->text[i]->dst.w) bigest_w = ddm->text[i]->dst.w;
		if (!ddm->static_h && bigest_h < ddm->text[i]->dst.h) bigest_h = ddm->text[i]->dst.h;
		ddm->text[i]->dst.y = ddm->default_pos.y + prev_height + 2;
		ddm->text[i]->dst.x = ddm->default_pos.x + 4;
		prev_height += ddm->text[i]->dst.h;
		ddm->text[i]->show = 0;
	}
	ddm->items -= 1;
	select_ddm_item(ddm, 0);
}

/* 	mabey split up in to a few functions but idk 	*/
void render_ddm(struct drop_down_menu *ddm)
{
	if (ddm == NULL) {
		printf("error passed a NULL pointer to render_ddm\n");
		return;
	}
	if (ddm->show == 0) return;
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(ddm->bg_color));
	if (ddm->selected == 0) {
		ddm->used_pos = ddm->default_pos;

		if (ddm->text[ddm->selected_text_index]->dst.h >= ddm->used_pos.h) 
			ddm->used_pos.h = ddm->text[ddm->selected_text_index]->dst.h + 2;

		SDL_RenderFillRect(renderer, &ddm->used_pos);

		for (int i = 0; i < ddm->items; i++) ddm->text[i]->show = i == ddm->selected_text_index ? 1 : 0;

		struct text tmp_text = { .texture = ddm->text[ddm->selected_text_index]->texture, .static_h = 0, .show = 1,
					 .static_w = 0, .dst = ddm->text[ddm->selected_text_index]->dst, 
					 .src = { 0, 0, ddm->text[ddm->selected_text_index]->dst.w,  ddm->text[ddm->selected_text_index]->dst.h } 
		};

		tmp_text.dst.x = ddm->default_pos.x + 4;
		tmp_text.dst.y = ddm->default_pos.y + 2;
		if (tmp_text.dst.x + tmp_text.dst.w > ddm->drop_pos.x + ddm->drop_pos.w) {
			tmp_text.src.w -= (tmp_text.dst.x + tmp_text.dst.w) - (ddm->default_pos.x + ddm->default_pos.w);
			tmp_text.dst.w -= (tmp_text.dst.x + tmp_text.dst.w) - (ddm->default_pos.x + ddm->default_pos.w);
		}

		render_text(&tmp_text, &tmp_text.src);

		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(ddm->outer_color));
		SDL_RenderDrawRect(renderer, &ddm->used_pos);
	} else {
		SDL_RenderFillRect(renderer, &ddm->drop_pos);
		int too_big = 0;
		for (int i = 0; i < ddm->items && !too_big; i++) {
			ddm->text[i]->show = 1;
			struct text tmp_text = { .show = 1, .texture = ddm->text[i]->texture, 
					.dst = ddm->text[i]->dst, .src = { 0, 0, ddm->text[i]->dst.w, ddm->text[i]->dst.h} 
			};

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
		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(ddm->outer_color));
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


struct slider *create_slider(int show, int x, int y, int w, int h, int button_size, void (*on_relase)(struct button *s, Event *e), void (*on_move)(struct slider *s, Event *e), color  button_fg_color, color  button_bg_color, color  bar_color, struct page *p)
{
	struct slider *return_slider = malloc(sizeof(struct slider));
	struct button *b = create_button(NULL, 1, show, x - button_size/2, y + h/2- button_size/2, button_size, button_size, 0, font, on_relase, NULL, button_fg_color, button_bg_color, button_fg_color, NULL);
	struct slider new_slider = { b, { x, y, w, h }, 0, show, 0, bar_color, on_move, p };

	memcpy(return_slider, &new_slider, sizeof(struct slider));
	if (p != NULL) {
		if (p->s_arr_used_len + 1 > p->s_arr_total_len) {
			p->s_arr = (struct slider**)realloc(p->s_arr, sizeof(struct slider*) * (p->s_arr_total_len + 5));
			p->s_arr_total_len += 5;
		}
		return_slider->index = p->s_arr_used_len;
		p->s_arr[p->s_arr_used_len] = return_slider;
		p->s_arr_used_len += 1;
	}

	return return_slider;
}

void destroy_slider(struct slider *slider)
{
	destroy_button(slider->button);
	if (slider->parent_p != NULL) {
		for (int i = slider->index; i < slider->parent_p->s_arr_used_len-1; i++) {
			slider->parent_p->s_arr[i] = slider->parent_p->s_arr[i+1];
			slider->parent_p->s_arr[i]->index = i;
		}
		slider->parent_p->s_arr[slider->parent_p->s_arr_used_len] = NULL;
		slider->parent_p->s_arr_used_len -= 1;
	}
	free(slider);

}

void render_slider(struct slider *slider)
{
	if (slider == NULL) {
		printf("error passed a NULL pointer to render_slider\n");
		return;
	}
	if (slider->show == 0) return;
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(slider->bar_color));
	SDL_RenderFillRect(renderer, &slider->pos);
	render_button(slider->button);
}

void update_slider(struct slider *slider, int x) 
{
	if      (x >= slider->pos.x + slider->pos.w) x = slider->pos.x + slider->pos.w;
	else if (x <= slider->pos.x) 		     x = slider->pos.x;
	slider->p = (float)(x-slider->pos.x)/(slider->pos.w);
	slider->button->pos.x = x - slider->button->pos.w/2;
}


struct graph *create_graph(int x, int y, int w, int h, int scale_w, int scale_h, int point_w, int point_h, int sp_w, int sp_h, int p_amount, void (*on_move)(), color  oc, color  bgc, color  fgc, color  point_c, color  sp_c, struct page *p)
{
	struct graph *return_graph = malloc(sizeof(struct graph));
	struct graph tmp_graph = { { x, y, w, h }, {x, y, w * scale_w, h * scale_h}, { 0, 0, point_w, point_h }, { 0, 0, sp_w, sp_h }, 
				   scale_w, scale_h, 0, 0, NULL, 0, 0, 1, p_amount, p_amount, NULL, { 0 }, on_move, NULL, oc, bgc, fgc, point_c, sp_c, p};
	struct point *pt = malloc(sizeof(struct point) * p_amount);
	tmp_graph.points = pt;
	memcpy(return_graph, &tmp_graph, sizeof(struct graph));
	if (p != NULL) {
		if (p->g_arr_used_len + 1 > p->g_arr_total_len) {
			p->g_arr = (struct graph**)realloc(p->g_arr, sizeof(struct graph*) * (p->g_arr_total_len + 5));
			p->g_arr_total_len += 5;
		}
		return_graph->index = p->g_arr_used_len;
		p->g_arr[p->g_arr_used_len] = return_graph;
		p->g_arr_used_len += 1;
	}

	return return_graph;
}

void destroy_graph(struct graph *graph)
{
	free(graph->points);
	if (graph->parent_p != NULL) {
		for (int i = graph->index; i < graph->parent_p->g_arr_used_len-1; i++) {
			graph->parent_p->g_arr[i] = graph->parent_p->g_arr[i+1];
			graph->parent_p->g_arr[i]->index = i;
		}
		graph->parent_p->g_arr[graph->parent_p->g_arr_used_len] = NULL;
		graph->parent_p->g_arr_used_len -= 1;
	}
	free(graph);
}

void render_graph(struct graph *graph)
{
	if (graph == NULL) {
		printf("error passed a NULL pointer to render_graph\n");
		return;
	}
	if (graph->show == 0) return;
	/* TODO come up with a better way */
	if (graph->point_amount <= 0) {
		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->bg_color));
		SDL_RenderFillRect(renderer, &graph->scaled_pos);
		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->outer_color));
		SDL_RenderDrawRect(renderer, &graph->scaled_pos);
		if (graph->x.x != 0) {
			SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->point_colors));
			SDL_RenderDrawLine(renderer, graph->real_pos.x + graph->x.x * graph->scale_w, graph->real_pos.y, 
							graph->real_pos.x + graph->x.x * graph->scale_w, graph->real_pos.y + graph->scaled_pos.h);
		}
		return;
	}
	int x1 = graph->points[0].x * graph->scale_w + graph->real_pos.x + graph->points_size.w/2;
	int y1 = graph->points[0].y * graph->scale_h + graph->real_pos.y + graph->points_size.h/2;
	int x2 = graph->points[graph->point_amount - 1].x * graph->scale_w + graph->real_pos.x + graph->points_size.w/2;
	int y2 = graph->points[graph->point_amount - 1].y * graph->scale_h + graph->real_pos.y + graph->points_size.h/2;

	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->bg_color));
	SDL_RenderFillRect(renderer, &graph->scaled_pos);

	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->fg_color));
	if (x2 > graph->scaled_pos.x + graph->scaled_pos.w) 
		x2 = graph->scaled_pos.x + graph->scaled_pos.w;
	if (y2 > graph->scaled_pos.y + graph->scaled_pos.h - graph->points_size.h/2) 
		y2 = graph->scaled_pos.y + graph->scaled_pos.h - graph->points_size.h/2;
	SDL_RenderDrawLine(renderer, x2, y2, graph->scaled_pos.x + graph->scaled_pos.w, y2);
	if (y1 > graph->scaled_pos.y + graph->scaled_pos.h - graph->points_size.h/2) 
		y1 = graph->scaled_pos.y + graph->scaled_pos.h - graph->points_size.h/2;

	SDL_RenderDrawLine(renderer, graph->scaled_pos.x, y1, x1, y1);
	for (int i = 0; i < graph->point_amount; i++) {
		if (i < graph->point_amount - 1) {
			SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->fg_color));

			x1 = graph->points[i].x * graph->scale_w + graph->real_pos.x + (graph->points_size.w/2);
			y1 = graph->points[i].y * graph->scale_h + graph->real_pos.y + graph->points_size.h/2;
			x2 = graph->points[i + 1].x * graph->scale_w + graph->real_pos.x + graph->points_size.w/2; 
			y2 = graph->points[i + 1].y * graph->scale_h + graph->real_pos.y + graph->points_size.h/2;

			if (x1 >= graph->scaled_pos.x + graph->scaled_pos.w) x1 = graph->scaled_pos.x + graph->scaled_pos.w;
			if (y1 >= graph->scaled_pos.y + graph->scaled_pos.h) y1 = graph->scaled_pos.y + graph->scaled_pos.h;
			if (x2 >= graph->scaled_pos.x + graph->scaled_pos.w) x2 = graph->scaled_pos.x + graph->scaled_pos.w;
			if (y2 >= graph->scaled_pos.y + graph->scaled_pos.h) y2 = graph->scaled_pos.y + graph->scaled_pos.h;

			SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
		}
		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->point_colors));
		rect_s tmp_rect = { graph->points[i].x * graph->scale_w + graph->real_pos.x, graph->points[i].y * graph->scale_h + graph->real_pos.y, graph->points_size.w, graph->points_size.h };

		if (tmp_rect.x + tmp_rect.w > graph->scaled_pos.x + graph->scaled_pos.w) tmp_rect.x = graph->scaled_pos.x + graph->scaled_pos.w - graph->points_size.w - 1;
		if (tmp_rect.y + tmp_rect.h > graph->scaled_pos.y + graph->scaled_pos.h) tmp_rect.y = graph->scaled_pos.y + graph->scaled_pos.h - graph->points_size.h - 1;

		SDL_RenderFillRect(renderer, &tmp_rect);
	}

	if (graph->selected_point != NULL) {
		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->selected_point_color));
		rect_s tmp_rect = { graph->selected_point->x * graph->scale_w + graph->scaled_pos.x, graph->selected_point->y * graph->scale_h + graph->scaled_pos.y, 
					graph->points_selected_size.w, graph->points_selected_size.h };

		if (tmp_rect.x + tmp_rect.w >= graph->scaled_pos.x + graph->scaled_pos.w) tmp_rect.x = graph->scaled_pos.x + graph->scaled_pos.w - graph->points_size.w - 1;
		if (tmp_rect.y + tmp_rect.h >= graph->scaled_pos.y + graph->scaled_pos.h) tmp_rect.y = graph->scaled_pos.y + graph->scaled_pos.h - graph->points_size.h - 1;

		SDL_RenderFillRect(renderer, &tmp_rect);
	}
	if (graph->x.x != 0) {
		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->point_colors));
		SDL_RenderDrawLine(renderer, graph->real_pos.x + graph->x.x * graph->scale_w, graph->real_pos.y, 
						graph->real_pos.x + graph->x.x * graph->scale_w, graph->real_pos.y + graph->scaled_pos.h);
	}
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->outer_color));
	SDL_RenderDrawRect(renderer, &graph->scaled_pos);

}

int copy_points(struct point *points, int *total_size, int *size, struct point *new_points, int new_size)
{
	PRINTINFO("copy_points\n");
	if (*total_size < new_size) {
		struct point *tmp = realloc(points, sizeof(struct point) * new_size);
		if (tmp == NULL) {
			printf("change_graph_points: failed to reallocate points new_size = %d\n", new_size);
			return -1;
		}
		points = tmp;
		*total_size = new_size;
	}
	memcpy(points, new_points, sizeof(struct point) * new_size);
	*size = new_size;
	return 0;
}

int change_graph_points(struct graph *g, struct point *new_points, int new_size)
{
	if (g->total_points < new_size) {
		struct point *tmp = realloc(g->points, sizeof(struct point) * new_size);
		if (tmp == NULL) {
			printf("change_graph_points: failed to reallocate points new_size = %d\n", new_size);
			return -1;
		}
		g->points = tmp;
		g->total_points = new_size;
	}
	memcpy(g->points, new_points, sizeof(struct point) * new_size);
	g->point_amount = new_size;
	return 0;
}

struct image *create_image(int x, int y, int w, int h, int sur_w, int sur_h, int sur_depth, struct page *p)
{
	struct image *return_image = malloc(sizeof(struct image));
	return_image->surface = SDL_CreateRGBSurface(0, sur_w, sur_h, sur_depth, 0, 0, 0, 0);
	return_image->pos.x = x;
	return_image->pos.y = y;
	return_image->pos.w = w;
	return_image->pos.h = h;

	if (p != NULL) {
		return_image->parent_p = p;
		return_image->show = p->show;
		if (p->img_arr_used_len + 1 > p->img_arr_total_len) {
			p->img_arr = (struct image**)realloc(p->img_arr, sizeof(struct image*) * (p->img_arr_total_len + 5));
			p->img_arr_total_len += 5;
		}
		return_image->index = p->g_arr_used_len;
		p->img_arr[p->img_arr_used_len] = return_image;
		p->img_arr_used_len += 1;
	} else {
		return_image->parent_p = NULL;
		return_image->show = 0;
		return_image->index = 0;
	}

	return return_image;
}

void destroy_image(struct image *img)
{
	if (img->parent_p != NULL) {
		for (int i = img->index; i < img->parent_p->img_arr_used_len-1; i++) {
			img->parent_p->img_arr[i] = img->parent_p->img_arr[i+1];
			img->parent_p->img_arr[i]->index = i;
		}
		img->parent_p->img_arr[img->parent_p->img_arr_used_len] = NULL;
		img->parent_p->img_arr_used_len -= 1;
	}
	SDL_FreeSurface(img->surface);
	SDL_DestroyTexture(img->texture);
	free(img);
}

void show_image(struct image *img)
{
	if (img->show != 0) SDL_RenderCopy(renderer, img->texture, NULL, &img->pos);
}

struct line *create_line(int x1, int y1, int x2, int y2, color  color, struct page *p)
{
	struct line *ret_line = malloc(sizeof(struct line));
	if (p != NULL) {
		ret_line->parent_p = p;
		ret_line->show = p->show;
		if (p->line_arr_used_len + 1 > p->line_arr_total_len) {
			p->line_arr = (struct line**)realloc(p->line_arr, sizeof(struct line*) * (p->line_arr_total_len + 5));
			p->line_arr_total_len += 5;
		}
		p->line_arr[p->line_arr_used_len] = ret_line;
		ret_line->index = p->line_arr_used_len;
		p->line_arr_used_len += 1;
	}
	ret_line->color = color;
	ret_line->start.x = x1; ret_line->start.y = y1;
	ret_line->too.x = x2; ret_line->too.y = y2;
	return ret_line;
}

void destroy_line(struct line *line)
{
	if (line == NULL) return;
	if (line->parent_p != NULL) {
		for (int i = line->index; i < line->parent_p->line_arr_used_len-1; i++) {
			line->parent_p->line_arr[i] = line->parent_p->line_arr[i+1];
			line->parent_p->line_arr[i]->index = i;
		}
		line->parent_p->line_arr[line->parent_p->line_arr_used_len] = NULL;
		line->parent_p->line_arr_used_len -= 1;
	}
	free(line);
}
void render_line(struct line *line)
{
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(line->color));
	SDL_RenderDrawLine(renderer, line->start.x, line->start.y, line->too.x, line->too.y);
}

void render_prompt(struct prompt *p)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
	rect_s bg = { 0, 0, WINDOW_W, WINDOW_H };
	SDL_RenderFillRect(renderer, &bg);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(p->bg_color));
	SDL_RenderFillRect(renderer, &p->pos);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(p->outer_color));
	SDL_RenderDrawRect(renderer, &p->pos);
	for (int i = 0; i < p->button_arr_used; i++) {
		render_button(p->button_arr[i]);
	}
	for (int i = 0; i < p->input_arr_used; i++) {
		render_input_box(p->input_arr[i]);
	}
	for (int i = 0; i < p->text_arr_used; i++) {
		render_text(p->text_arr[i], NULL);
	}
}

void show_prompt(struct prompt *p)
{
	if (showen_prompt != NULL) showen_prompt->show = 0;
	showen_prompt = p;
	if (showen_prompt != NULL) showen_prompt->show = 1;
	if (showen_prompt != NULL && showen_prompt->on_show != NULL) showen_prompt->on_show();
}

void add_button_to_prompt(struct prompt *p, struct button *b)
{
	b->pos.x += p->pos.x;
	b->pos.y += p->pos.y;
	b->text->dst.x += p->pos.x;
	b->text->dst.y += p->pos.y;
	b->index = p->button_arr_used;
	if (p->button_arr_total < p->button_arr_used + 1) {
		struct button **tmp = realloc(p->button_arr, sizeof(struct button *) * (p->button_arr_total + 5));
		if (tmp == NULL) {
			printf("add_button_to_prompt: failed to reallocate p->button_arr\n");
			return;
		}
		p->button_arr_total += 5;
		p->button_arr = tmp;
	}
	p->button_arr[p->button_arr_used] = b;
	p->button_arr_used++;
}

void add_input_to_prompt(struct prompt *p, struct input *i)
{
	i->default_pos.x += p->pos.x;
	i->default_pos.y += p->pos.y;
	i->pos.x += p->pos.x;
	i->pos.y += p->pos.y;
	i->text->dst.x += p->pos.x;
	i->text->dst.y += p->pos.y;
	i->default_text->dst.x += p->pos.x;
	i->default_text->dst.y += p->pos.y;
	i->index = p->input_arr_used;
	if (p->input_arr_total < p->input_arr_used + 1) {
		struct input **tmp = realloc(p->input_arr, sizeof(struct input *) * (p->input_arr_total + 5));
		if (tmp == NULL) {
			printf("add_input_to_prompt: failed to reallocate p->input_arr\n");
			return;
		}
		p->input_arr_total += 5;
		p->input_arr = tmp;
	}
	p->input_arr[p->input_arr_used] = i;
	p->input_arr_used++;
}

void add_text_to_prompt(struct prompt *p, struct text *t)
{
	t->dst.x += p->pos.x;
	t->dst.y += p->pos.y;
	t->index = p->text_arr_used;
	if (p->text_arr_total < p->text_arr_used + 1) {
		struct text **tmp = realloc(p->text_arr, sizeof(struct text *) * (p->text_arr_total + 5));
		if (tmp == NULL) {
			printf("add_text_to_prompt: failed to reallocate p->text_arr\n");
			return;
		}
		p->text_arr_total += 5;
		p->text_arr = tmp;
	}
	p->text_arr[p->text_arr_used] = t;
	p->text_arr_used++;
}

void destroy_prompt(struct prompt *p)
{
	if (p == NULL) return;
	PRINTINFO("destroy_prompt:\n");
	while (p->button_arr_used > 0) {
		p->button_arr_used -= 1;
		destroy_button(p->button_arr[p->button_arr_used]);
	}
	while (p->input_arr_used > 0) {
		p->input_arr_used -= 1;
		destroy_input_box(p->input_arr[p->input_arr_used]);
	}
	while (p->text_arr_used > 0) {
		p->text_arr_used -= 1;
		destroy_text(p->text_arr[p->text_arr_used]);
	}
	for (int i = p->index; i < prompt_arr_len-1; i++) {
		prompt_arr[i] = prompt_arr[i + 1];
		prompt_arr[i]->index = i;
	}
	free(p->button_arr);
	free(p->input_arr);
	free(p->text_arr);
	free(p);
	prompt_arr_len -= 1;
	prompt_arr[prompt_arr_len] = NULL;
}

struct prompt *create_prompt(int x, int y, int w, int h, color  bg_color, color  outer_color, TTF_Font *font)
{
	struct prompt *ret_prompt = malloc(sizeof(struct prompt));
	struct prompt tmp_prompt = { { x, y, w, h }, 0, prompt_arr_len, NULL, NULL, 0, 0, NULL, NULL, 0, 0, NULL, NULL, 0, 0, bg_color, outer_color, font };
	tmp_prompt.text_arr = malloc(sizeof(struct button *) * 5);
	tmp_prompt.text_arr_total = 5;
	tmp_prompt.input_arr = malloc(sizeof(struct button *) * 5);
	tmp_prompt.input_arr_total = 5;
	tmp_prompt.button_arr = malloc(sizeof(struct button *) * 5);
	tmp_prompt.button_arr_total = 5;
	struct prompt **tmp = realloc(prompt_arr, sizeof(struct prompt *) * (prompt_arr_len + 1));
	if (tmp == NULL) {
		printf("create_prompt: failed to reallocate prompt_arr\n");
		free(tmp_prompt.text_arr);
		free(tmp_prompt.input_arr);
		free(tmp_prompt.button_arr);
		free(ret_prompt);
		return NULL;
	}
	prompt_arr = tmp;
	memcpy(ret_prompt, &tmp_prompt, sizeof(struct prompt));
	prompt_arr[prompt_arr_len] = ret_prompt;
	prompt_arr_len++;
	return prompt_arr[prompt_arr_len-1];
}



