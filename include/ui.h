#ifndef UI_H
#define UI_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_W 800
#define WINDOW_H 500

#define MAX_TEXT_SIZE 256

#define SDL_COLOR_ARG(C) C.r, C.g, C.b, C.a
#define CHECK_RECT(r1, r2) r1.x < r2.x + r2.w && r1.x > r2.x && r1.y < r2.y + r2.h && r1.y > r2.y

SDL_Color BLACK   = {   0,   0,   0, SDL_ALPHA_OPAQUE };
SDL_Color WHITE   = { 255, 255, 255, SDL_ALPHA_OPAQUE };
SDL_Color RED     = { 255,   0,   0, SDL_ALPHA_OPAQUE };
SDL_Color GREEN   = {   0, 255,   0, SDL_ALPHA_OPAQUE };
SDL_Color BLUE    = {   0,   0, 255, SDL_ALPHA_OPAQUE };

char font_path[128];
TTF_Font *font;

int running;
SDL_Window   *window;
SDL_Renderer *renderer;
SDL_Event event;
static int mouse_x, mouse_y;

struct image {
	SDL_Texture *texture;
	SDL_Surface *surface;
	SDL_Rect pos;
	int index, show;
	struct page *parent_p;
};

struct point {
	int x, y;
};

struct text {
	char str[MAX_TEXT_SIZE];
	int show, static_w, static_h, font_size, wrap_length;
	SDL_Color fg_color, bg_color;
	SDL_Rect src, dst;
	SDL_Texture *texture;
	int index; /* private */
	struct page *parent_p;
};
static int default_font_size = 20;

struct button {
	struct text *text;
	SDL_Texture **texture;
	void (*on_click) (struct button *self, SDL_Event *event);
	void (*on_move) (struct button *self, SDL_Event *event);
	int clickable, movable, show;
	int index; /* private */
	SDL_Rect outer_box;
	SDL_Color outer_box_color, bg_color;
	struct page *parent_p;
};

struct slider {
	struct button *button;
	SDL_Rect pos;
	float p;
	int show;
	int index; /* private */
	SDL_Color bar_color;
	void (*on_move)();
	struct page *parent_p;
};

struct input {
	int index, selected, max_len, show;
	struct text *text;
	int resize_box;
	void (*function)(struct input *self, SDL_Event *event);
	SDL_Rect default_outer_box, outer_box;
	SDL_Color outer_box_color, bg_color;

	struct page *parent_p;
};

struct drop_down_menu {
	int selected, selected_text_index, update_highlight, show;
	int index, static_w, static_h; /* private */
	int items;
	struct text **text;
	void (*function)(struct drop_down_menu *self, SDL_Event *event);
	int scroll_offset;
	SDL_Rect default_pos, used_pos, drop_pos, highlight_pos;
	SDL_Color outer_box_color, bg_color;
	struct page *parent_p;
};

struct graph {
	SDL_Rect real_pos, scaled_pos;
	SDL_Rect points_size, points_selected_size;
	int scale_w, scale_h;
	int selected, selected_point_index;
	struct point *selected_point;
	int index, rerender, show;
	int point_amount;
	struct point *points;
	void (*on_move)(struct graph *self, SDL_Event *e);
	SDL_Color outer_color, bg_color, fg_color, point_colors, selected_point_color;
	struct page *parent_p;
};

struct page {
	int show;
	struct text **t_arr;
	int t_arr_total_len, t_arr_used_len;
	struct button **b_arr, *selected_b;
	int b_arr_total_len, b_arr_used_len;
	struct slider **s_arr, *selected_s;
	int s_arr_total_len, s_arr_used_len;
	struct input **i_arr, *selected_i;
	int i_arr_total_len, i_arr_used_len;
	struct drop_down_menu **d_arr, *selected_d;
	int d_arr_total_len, d_arr_used_len;
	struct graph **g_arr, *selected_g;
	int g_arr_total_len, g_arr_used_len;
	struct image **img_arr;
	int img_arr_total_len, img_arr_used_len;
};
static struct page **page_arr;
static struct page *showen_page;
static int page_arr_total_len;
static int page_arr_used_len;

int ui_init();
void ui_shutdown();
void handle_event(SDL_Event *event);
void lmouse_button_down(SDL_Event *event);
void lmouse_button_up(SDL_Event *event);
void mouse_wheel(SDL_MouseWheelEvent *event);
void mouse_move(SDL_Event *event);
void show_screen();
void clear_screen(SDL_Color color);

struct image *create_image(int x, int y, int w, int h, int sur_w, int sur_h, int sur_depth, struct page *p);
void destroy_image(struct image *img);
void show_image(struct image *img);
SDL_Texture *create_texture_from_surface(SDL_Surface *sur);

struct page *create_page();
void destroy_page(struct page *page);
void show_page(struct page *page);
void render_showen_page();

struct text *create_text(char *string, int x, int y, int w, int h, int font_size, int wrap_length, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f, struct page *p);
void destroy_text_texture(struct text *text);
void destroy_text(struct text *text);
void render_text(struct text *t, SDL_Rect *src);
int render_text_texture(struct text *t, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f);
void change_text_and_render_texture(struct text *text, char *new_text, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f);

struct button *create_button(char *string, int movable, int show, int x, int y, int w, int h, int font_size, TTF_Font *f, void (*on_click)(), void (*on_move)(), SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color, struct page *p);
void destroy_button(struct button *button);
void render_button(struct button *button);

struct slider *create_slider(int show, int x, int y, int w, int h, int button_size, void (*on_relase)(), void (*on_move)(), SDL_Color button_fg_color, SDL_Color button_bg_color, SDL_Color bar_color, struct page *p);
void render_slider(struct slider *slider);
void update_slider(struct slider *slider, int x);
void destroy_slider(struct slider *slider);

struct input *create_input(char *text, int resize_box, int max_len, int x, int y, int w, int h, void (*function)(struct input *self, SDL_Event *event), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color, struct page *p);
void destroy_input_box(struct input *input_box);
void change_input_box_text(struct input *input_box, char *str);
void render_input_box(struct input *input_box);

struct drop_down_menu *create_drop_down_menu(int items, char item_str[][MAX_TEXT_SIZE], int x, int y, int w, int h, int dw, int dh, void (*function)(), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color tc, struct page *p);
void destroy_ddm(struct drop_down_menu *ddm);
void render_ddm(struct drop_down_menu *ddm);
void change_ddm_text_arr(struct drop_down_menu *ddm, int items, char newstr[][MAX_TEXT_SIZE], TTF_Font *f);
void update_ddm_highlight(int x, int y, struct drop_down_menu *ddm);

struct graph *create_graph(int x, int y, int w, int h, int scalew, int scaleh, int pw, int ph, int spw, int sph, int pamount, void (*on_move)(struct graph *self, SDL_Event *e), SDL_Color oc, SDL_Color bgc, SDL_Color fgc, SDL_Color pointc, SDL_Color spc, struct page *p);
void destroy_graph(struct graph *graph);
void render_graph(struct graph *graph);
void change_graph_point(struct graph *graph);

#endif
