#include "ui.h"

int running;

SDL_Window   *window;
SDL_Renderer *renderer;
TTF_Font     *font;
char font_path[128];
int default_font_size = 20;
struct line *cursor;

SDL_Color BLACK   = {   0,   0,   0, SDL_ALPHA_OPAQUE };
SDL_Color WHITE   = { 255, 255, 255, SDL_ALPHA_OPAQUE };
SDL_Color RED     = { 255,   0,   0, SDL_ALPHA_OPAQUE };
SDL_Color GREEN   = {   0, 255,   0, SDL_ALPHA_OPAQUE };
SDL_Color BLUE    = {   0,   0, 255, SDL_ALPHA_OPAQUE };


static int mouse_x, mouse_y;

static struct page **page_arr;
struct page *showen_page;
static int page_arr_total_len;
static int page_arr_used_len;

static void lmouse_button_down(SDL_Event *event);
static void lmouse_button_up(SDL_Event *event);
static void rmouse_button_down(SDL_Event *event);
static void rmouse_button_up(SDL_Event *event);
static void mouse_wheel(SDL_MouseWheelEvent *event);
static void mouse_move(SDL_Event *event);

int get_default_fontpath(void)
{
	FcInit();

	FcConfig *c = FcInitLoadConfigAndFonts();
	FcPattern *p = FcNameParse((const FcChar8 *)"HackNerdFont");
	FcConfigSubstitute(c, p, FcMatchPattern);
	FcDefaultSubstitute(p);

	char *font_p = NULL;
	FcResult res;
	FcPattern *font = FcFontMatch(c, p, &res);

	int err = -1;
	if (font) {
		FcChar8 *file = NULL;
		if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch){
			font_p = (char *)file;
			strcpy(font_path, font_p);
			printf("%s, %s\n", font_p, font_path);
			err = 0;
		}
			
	}

	FcPatternDestroy(font);
	FcPatternDestroy(p);
	FcConfigDestroy(c);
	FcFini();
	return err;
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
	//strcpy(font_path, get_default_fontpath(), 128);
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
	TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_LEFT);

	page_arr = malloc(sizeof(struct page *) * 5);
	page_arr_total_len = 5;
	page_arr_used_len = 0;
	showen_page = NULL;

	cursor = create_line(0, 0, 0, 0, WHITE, NULL);

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
	free(page_arr);
	TTF_Quit();
	SDL_Quit();
}

void show_screen(void)
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
					if (showen_page->selected_i != NULL) {
						size_t len = strlen(showen_page->selected_i->text->str);
						char tmpstr[MAX_TEXT_SIZE];
						strcpy(tmpstr, showen_page->selected_i->text->str);
						tmpstr[len-1] = '\0';
						change_input_box_text(showen_page->selected_i, tmpstr);
					}
					break;
				default:
					break;
			}
			break;
		case SDL_TEXTINPUT:
			if (showen_page->selected_i!= NULL) {
				size_t len = strlen(showen_page->selected_i->text->str);
				if (len >= showen_page->selected_i->max_len) break;
				if (showen_page->selected_i->max_len > 0) {
					char tmpstr[MAX_TEXT_SIZE]; 
					if (showen_page->selected_i->filter != NULL && showen_page->selected_i->filter(showen_page->selected_i, event->text.text) == 0) {
						strncpy(tmpstr, showen_page->selected_i->text->str, MAX_TEXT_SIZE);
						strncat(tmpstr, event->text.text, MAX_TEXT_SIZE - len);
					} else if (showen_page->selected_i->filter == NULL) {
						strncpy(tmpstr, showen_page->selected_i->text->str, MAX_TEXT_SIZE);
						strncat(tmpstr, event->text.text, MAX_TEXT_SIZE - len);
					} else {
						strncpy(tmpstr, showen_page->selected_i->text->str, MAX_TEXT_SIZE);
					}
					change_input_box_text(showen_page->selected_i, tmpstr);
				}
			}
			break;
		default:
			break;
	}
}

static void mouse_wheel(SDL_MouseWheelEvent *event)
{
	int wheely = event->y;
	SDL_Rect mouse_pos = { event->mouseX, event->mouseY, 0 };
	if (showen_page->selected_d != NULL && CHECK_RECT(mouse_pos, showen_page->selected_d->drop_pos) ) {
		int lty = showen_page->selected_d->text[showen_page->selected_d->items-1]->dst.y; 
		int lth = showen_page->selected_d->text[showen_page->selected_d->items-1]->dst.h;
		int dpy = showen_page->selected_d->drop_pos.y, dph = showen_page->selected_d->drop_pos.h;

		showen_page->selected_d->scroll_offset += wheely * 10;
		showen_page->selected_d->update_highlight = 1;
		if (showen_page->selected_d->scroll_offset > 0) 
			showen_page->selected_d->scroll_offset = 0;
		else if (showen_page->selected_d->scroll_offset < -(lty + lth - dpy - dph)) 
			showen_page->selected_d->scroll_offset = -(lty + lth - dpy - dph);

	}
}

static void mouse_move(SDL_Event *event)
{
	mouse_x = event->motion.x;
	mouse_y = event->motion.y;
	if (showen_page->selected_b != NULL && showen_page->selected_b->movable == 1) {
		showen_page->selected_b->on_move(showen_page->selected_b, event);
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

static void rmouse_button_down(SDL_Event *event)
{
	
}

static void rmouse_button_up(SDL_Event *event)
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

/* LATER: only check if button or input_box is visible mabey create array with all showen things idk */
static void lmouse_button_up(SDL_Event *event)
{
	SDL_MouseButtonEvent mouse_data = event->button;
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
		if (CHECK_RECT(mouse_data, showen_page->i_arr[i]->outer_box))  {
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
			SDL_StopTextInput();
		}
	}
	if (hit == 0) {
		showen_page->selected_i = NULL;
		cursor->show = 0;
	}
	hit = 0;
	if (showen_page->selected_d != NULL && CHECK_RECT(mouse_data, showen_page->selected_d->drop_pos)) {
		for (int i = 0; i < showen_page->selected_d->items; i++) {
			SDL_Rect pos = showen_page->selected_d->text[i]->dst;
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

void select_ddm_item(struct drop_down_menu *ddm, int item)
{
	ddm->text[ddm->selected_text_index]->show = 0;
	ddm->selected_text_index = item;
	ddm->text[item]->show = 1;
	ddm->scroll_offset = -(ddm->text[item]->dst.y - ddm->used_pos.y - 2);
}

static void lmouse_button_down(SDL_Event *event)
{
	SDL_MouseButtonEvent mouse_data = event->button;
	for (int i = 0; i < showen_page->b_arr_used_len; i++) {
		if (showen_page->b_arr[i]->clickable == 1 && CHECK_RECT(event->button, showen_page->b_arr[i]->outer_box)) {
			showen_page->selected_b = showen_page->b_arr[i];
		}
	}
	for (int i = 0; i < showen_page->s_arr_used_len; i++) {
		if (CHECK_RECT(mouse_data, showen_page->s_arr[i]->button->outer_box)) {
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
	page_arr[page_arr_used_len] = return_page;
	page_arr_used_len += 1;

	return return_page;
}

void destroy_page(struct page *page)
{
	struct input **tmp_iarr = page->i_arr;
	struct button **tmp_barr = page->b_arr;
	struct drop_down_menu **tmp_darr = page->d_arr;
	struct slider **tmp_sarr = page->s_arr;
	struct text **tmp_tarr = page->t_arr;
	struct graph **tmp_garr = page->g_arr;
	for (int i = 0; i < page->img_arr_used_len; i++) {
		destroy_image(page->img_arr[page->img_arr_used_len-1]);
	}
	while (page->i_arr_used_len > 0) {
		destroy_input_box(tmp_iarr[page->i_arr_used_len-1]);
	}
	while (page->b_arr_used_len > 0) {
		destroy_button(tmp_barr[page->b_arr_used_len-1]);
	}
	while (page->d_arr_used_len > 0) {
		destroy_ddm(tmp_darr[page->d_arr_used_len-1]);
	}
	while (page->s_arr_used_len > 0) {
		destroy_slider(tmp_sarr[page->s_arr_used_len-1]);
	}
	while (page->t_arr_used_len > 0) {
		destroy_text(tmp_tarr[page->t_arr_used_len-1]);
	}
	while (page->g_arr_used_len > 0) {
		destroy_graph(tmp_garr[page->g_arr_used_len-1]);
	}
	while (page->line_arr_used_len > 0) {
		destroy_line(page->line_arr[page->line_arr_used_len-1]);
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
	if (showen_page != NULL) showen_page->show = 0;
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
	if (cursor->show != 0) render_line(cursor);
	for (int i = 0; i < showen_page->s_arr_used_len; i++) {
		if (showen_page->s_arr[i]->show != 0) render_slider(showen_page->s_arr[i]);
	}
	for (int i = 0; i < showen_page->d_arr_used_len; i++) {
		if (showen_page->d_arr[i]->show != 0) render_ddm(showen_page->d_arr[i]);
	}
	for (int i = 0; i < showen_page->line_arr_used_len; i++) {
		if (showen_page->line_arr[i]->show != 0) render_line(showen_page->line_arr[i]);
	}
}

struct text *create_text(char *string, int x, int y, int w, int h, int font_size, int wrap_length, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f, struct page *p)
{
	struct text *return_text = malloc(sizeof(struct text));
	struct text new_text = { "", 1, 0, 0, 0, wrap_length, fg_color, bg_color, { 0 }, { x, y, 0, 0 }, NULL, 0, p };
	
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
	if (text == NULL) return;
	destroy_text_texture(text);
	if (text->parent_p != NULL) {
		for (int i = text->index; i < text->parent_p->t_arr_used_len-1; i++) {
			text->parent_p->t_arr[i] = text->parent_p->t_arr[i+1];
			text->parent_p->t_arr[i]->index = i;
		}
		text->parent_p->t_arr[text->parent_p->t_arr_used_len] = NULL;
		text->parent_p->t_arr_used_len -= 1;
	}
	free(text);
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

struct button *create_button(char *string, int movable, int show, int x, int y, int w, int h, int font_size, TTF_Font *f, void (*on_click)(), void (*on_move)(), SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color, struct page *p)
{
	struct button *return_button = malloc(sizeof(struct button));
	struct button new_button = { NULL, NULL, on_click, on_move, 0, movable, show, 0, { x, y, }, outer_color, bg_color, p };

	if (on_click != NULL) new_button.clickable = 1;
	if (string != NULL) {
		int tmp_w = 0, tmp_h = 0;
		if (h != 0) tmp_h = h - 10;
		if (w != 0) tmp_w = w - 10;
		new_button.text = create_text(string, x + 5, y + 5, tmp_w, tmp_h, font_size, 0, text_color, bg_color, f, NULL);
		new_button.texture = &new_button.text->texture;
	} 
	new_button.outer_box.w = w == 0 ? new_button.text->dst.w + 10 : w;
	new_button.outer_box.h = h == 0 ? new_button.text->dst.h + 10 : h;
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
	if (button->text != NULL) destroy_text(button->text);

	if (button->parent_p != NULL) {
		for (int i = button->index; i < button->parent_p->b_arr_used_len-1; i++) {
			button->parent_p->b_arr[i] = button->parent_p->b_arr[i+1];
			button->parent_p->b_arr[i]->index = i;
		}
		button->parent_p->b_arr[button->parent_p->b_arr_used_len] = NULL;
		button->parent_p->b_arr_used_len -= 1;
	}
	free(button);
}
void render_button(struct button *button)
{
	if (button == NULL) {
		printf("error passed a NULL pointer to render_button\n");
		return;
	}
	if (button->show == 0 ) return;
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(button->bg_color));
	SDL_RenderFillRect(renderer, &button->outer_box);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(button->outer_box_color));
	SDL_RenderDrawRect(renderer, &button->outer_box);
	if (button->text != NULL) render_text(button->text, NULL);
}

// 	   |---------|
// 	-> |some text|
// 	   |---------|
struct input *create_input(char *string, char *def_text, int resize_box, int max_len, int x, int y, int w, int h, void (*function)(), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color, struct page *p)
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
			new_input.text = create_text(tstr, x+5, y+5, 0, 0, 20, 0, text_color, bg_color, f, NULL);
			w = new_input.text->dst.w + 10;
			h = new_input.text->dst.h + 10;
			tstr[1] = '\0';
			change_text_and_render_texture(new_input.text, tstr, text_color, bg_color, f);
			new_input.char_size.w = new_input.text->dst.w;
			new_input.char_size.h = new_input.text->dst.h;
			change_text_and_render_texture(new_input.text, string, text_color, bg_color, f);
		}
		else new_input.text = create_text(string, x+5, y+5, 0, 0, 20, 0, text_color, bg_color, f, NULL);
	}
	if (def_text != NULL) {
		new_input.default_text = create_text(def_text, x+5, y+5, 0, 0, 20, 0, text_color, bg_color, f, NULL);
	}

	if (w == 0) {
		new_input.default_outer_box.w = new_input.outer_box.w = new_input.text->dst.w + 10;
	} else new_input.default_outer_box.w = new_input.outer_box.w = w;
	if (h == 0) {
		new_input.default_outer_box.h = new_input.outer_box.h = new_input.text->dst.h + 10;
	} else new_input.default_outer_box.h = new_input.outer_box.h = h;

	if (!resize_box) {
		new_input.text->src = new_input.text->dst;
		new_input.text->src.x = 0;
		new_input.text->src.y = 0;
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
	SDL_Rect prev_input_margin = input_box->outer_box;
	prev_input_margin.w = input_box->outer_box.w - input_box->text->dst.w;
	prev_input_margin.h = input_box->outer_box.h - input_box->text->dst.h;

	size_t old_len = strlen(input_box->text->str), new_len = strlen(str);
	if (old_len < new_len) {
		cursor->too.x = cursor->start.x += input_box->char_size.w;
	} else if (old_len > new_len) {
		cursor->too.x = cursor->start.x -= input_box->char_size.w;
	}
	strncpy(input_box->text->str, str, MAX_TEXT_SIZE);
	if (str[0] != '\0') {
		render_text_texture(input_box->text, input_box->text->fg_color, input_box->text->bg_color, font);
		input_box->text->show = 1;
		input_box->default_text->show = 0;
	} else {
		render_text_texture(input_box->default_text, input_box->default_text->fg_color, input_box->default_text->bg_color, font);
		input_box->text->show = 0;
		input_box->default_text->show = 1;
	}

	if (input_box->resize_box == 1) {
		input_box->text->src.x = 0;
		input_box->text->src.y = 0;
		input_box->text->src.w = input_box->text->dst.w;
		input_box->text->src.h = input_box->text->dst.h;
		input_box->outer_box.w = input_box->text->dst.w + prev_input_margin.w;
		input_box->outer_box.h = input_box->text->dst.h + prev_input_margin.h;
	} else {
		input_box->text->src.w = input_box->outer_box.w = input_box->default_outer_box.w;
		input_box->text->src.h = input_box->outer_box.h = input_box->default_outer_box.h;
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
	SDL_RenderFillRect(renderer, &input_box->outer_box);
	SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(input_box->outer_box_color));
	SDL_RenderDrawRect(renderer, &input_box->outer_box);
	if (input_box->text->str[0] != '\0') render_text(input_box->text, &input_box->text->src);
}

void destroy_input_box(struct input *input_box)
{
	destroy_text(input_box->text);
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

struct drop_down_menu *create_drop_down_menu(int items, char item_str[][MAX_TEXT_SIZE], int x, int y, int w, int h, int dw, int dh, void (*function)(), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color tc, struct page *p)
{
	SDL_Rect pos = { x, y, w, h };
	SDL_Rect dpos = { x, y, dw, dh };

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
		ddm.text[i] = create_text(item_str[i], 0, 0, 0, 0, 0, w, tc, bg_color, f, NULL);
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

	SDL_Color tc = ddm->text[0]->fg_color;
	int prev_height = 0;
	int bigest_w = ddm->default_pos.w, bigest_h = ddm->default_pos.h;
	for (int i = 0; i < items; i++) {
		ddm->text[i] = create_text(newstr[i], 0, 0, 0, 0, 0, ddm->default_pos.w, tc, ddm->bg_color, f, NULL);
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

		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(ddm->outer_box_color));
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


struct slider *create_slider(int show, int x, int y, int w, int h, int button_size, void (*on_relase)(struct button *s, SDL_Event *e), void (*on_move)(struct slider *s, SDL_Event *e), SDL_Color button_fg_color, SDL_Color button_bg_color, SDL_Color bar_color, struct page *p)
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
	slider->button->outer_box.x = x - slider->button->outer_box.w/2;
}


struct graph *create_graph(int x, int y, int w, int h, int scale_w, int scale_h, int point_w, int point_h, int sp_w, int sp_h, int p_amount, void (*on_move)(), SDL_Color oc, SDL_Color bgc, SDL_Color fgc, SDL_Color point_c, SDL_Color sp_c, struct page *p)
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
		SDL_Rect tmp_rect = { graph->points[i].x * graph->scale_w + graph->real_pos.x, graph->points[i].y * graph->scale_h + graph->real_pos.y, graph->points_size.w, graph->points_size.h };

		if (tmp_rect.x + tmp_rect.w > graph->scaled_pos.x + graph->scaled_pos.w) tmp_rect.x = graph->scaled_pos.x + graph->scaled_pos.w - graph->points_size.w - 1;
		if (tmp_rect.y + tmp_rect.h > graph->scaled_pos.y + graph->scaled_pos.h) tmp_rect.y = graph->scaled_pos.y + graph->scaled_pos.h - graph->points_size.h - 1;

		SDL_RenderFillRect(renderer, &tmp_rect);
	}

	if (graph->selected_point != NULL) {
		SDL_SetRenderDrawColor(renderer, SDL_COLOR_ARG(graph->selected_point_color));
		SDL_Rect tmp_rect = { graph->selected_point->x * graph->scale_w + graph->scaled_pos.x, graph->selected_point->y * graph->scale_h + graph->scaled_pos.y, 
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

struct line *create_line(int x1, int y1, int x2, int y2, SDL_Color color, struct page *p)
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




