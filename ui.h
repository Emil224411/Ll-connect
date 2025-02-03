#ifndef UI_H
#define UI_H
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

//#define INFO

#ifdef INFO
#define PRINTINFO_VA(fmt, ...) printf(fmt, __VA_ARGS__)
#define PRINTINFO(str) printf(str)
#else
#define PRINTINFO_VA(fmt, ...)
#define PRINTINFO(str)
#endif

#define WINDOW_W 800
#define WINDOW_H 500

#define MAX_TEXT_SIZE 256

#define SDL_COLOR_ARG(C) C.r, C.g, C.b, C.a
#define CHECK_RECT(r1, r2) r1.x < r2.x + r2.w && r1.x > r2.x && r1.y < r2.y + r2.h && r1.y > r2.y

typedef SDL_Color color;
typedef SDL_Texture texture_s;
typedef SDL_Surface surface_s;
typedef SDL_Rect rect_s;
typedef SDL_Event Event;

struct callback {
	void (*function)(void);
	unsigned int a, b;
	double timer;
	int index, que_index, is_qued;
	int times_called_back;
};

struct image {
	texture_s *texture;
	surface_s *surface;
	rect_s pos;
	int index, show;
	struct page *parent_p;
};

struct point {
	int x, y;
};

struct text {
	char str[MAX_TEXT_SIZE];
	int show, static_w, static_h, font_size, wrap_length;
	color fg_color;
	rect_s src, dst;
	texture_s *texture;
	int index; /* private */
	struct page *parent_p;
};

struct button {
	struct text *text;
	texture_s **texture;
	void (*on_click) (struct button *self, Event *event);
	void (*on_move) (struct button *self, Event *event);
	void (*on_hover) (struct button *self, Event *event);
	int clickable, movable, hoverable, hovering, show;
	int index; /* private */
	rect_s pos;
	color outer_color, bg_color;
	struct page *parent_p;
};

struct slider {
	struct button *button;
	rect_s pos;
	float p;
	int show;
	int index; /* private */
	color bar_color;
	void (*on_move)(struct slider *self, Event *event);
	struct page *parent_p;
};

struct input {
	int index, selected, max_len, show;
	struct text *text, *default_text;
	int resize_box;
	void (*on_type)(struct input *self, Event *event);
	int (*filter)(struct input *self, char new_text[32]);
	rect_s default_pos, pos;
	color outer_color, bg_color;
	rect_s char_size;
	struct page *parent_p;
};

struct drop_down_menu {
	int selected, selected_text_index, update_highlight, show;
	int index, static_w, static_h; /* private */
	int items;
	struct text **text;
	void (*function)(struct drop_down_menu *self, Event *event);
	int scroll_offset;
	rect_s default_pos, used_pos, drop_pos, highlight_pos;
	color outer_color, bg_color;
	struct page *parent_p;
};

struct graph {
	rect_s real_pos, scaled_pos;
	rect_s points_size, points_selected_size;
	int scale_w, scale_h;
	int selected, selected_point_index;
	struct point *selected_point;
	int index, rerender, show;
	int point_amount, total_points;
	struct point *points;
	struct point x;
	void (*on_move)(struct graph *self, Event *e);
	void (*on_click)(struct graph *self, Event *e);
	color  outer_color, bg_color, fg_color, point_colors, selected_point_color;
	struct page *parent_p;
};

struct line {
	color  color;
	int show, index;
	struct point start, too;
	struct page *parent_p;
};

struct prompt {
	rect_s pos;
	int show, index;
	void (*on_show)(void);
	struct text **text_arr;
	int text_arr_total, text_arr_used;
	struct input **input_arr, *selected_input;
	int input_arr_total, input_arr_used;
	struct button **button_arr, *selected_button;
	int button_arr_total, button_arr_used;
	color  bg_color, outer_color;
	TTF_Font *font;
};

struct page {
	char test_name[32];
	int show, index;
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
	struct line **line_arr;
	int line_arr_total_len, line_arr_used_len;
};

/* global variables */
extern color BLACK;
extern color WHITE;
extern color RED;
extern color GREEN;
extern color BLUE;

extern struct page *showen_page;
extern struct prompt *showen_prompt;
extern struct line *cursor;
extern int running;
extern char font_path[128];
extern TTF_Font *font;
extern SDL_Window   *window;
extern SDL_Renderer *renderer;

/* functions */
Uint32 get_ticks(void);
void destroy_texture_s(texture_s *t);
surface_s *create_RGB_surface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
void delay(Uint32 ms);
int  ui_init(void);
void ui_shutdown(void);
void check_events_and_callbacks(Event *event);
void handle_event(Event *event);
void show_screen(void);
void clear_screen(color  color);
int get_default_fontpath(void);

/* callback functions */
double get_time_till_cb(struct callback *cb);
void set_callback_timer(struct callback *cb, double time);
struct callback *create_callback(void (*function)(void), double time);
void destroy_callback(struct callback *cb);
void check_next_callback(void);
void remove_from_que(struct callback *cb);
void check_callbacks(void);
void add_callback_to_que(struct callback *cb);

/* prompt functions */
struct prompt *create_prompt(int x, int y, int w, int h, color  bg_color, color  outer_color, TTF_Font *f);
void destroy_prompt(struct prompt *p);
void render_prompt(struct prompt *p);
void show_prompt(struct prompt *p);
void add_button_to_prompt(struct prompt *p, struct button *b);
void add_input_to_prompt(struct prompt *p, struct input *i);
void add_text_to_prompt(struct prompt *p, struct text *t);

/* image functions */
struct image *create_image(int x, int y, int w, int h, int sur_w, int sur_h, int sur_depth, struct page *p);
void destroy_image(struct image *img);
void show_image(struct image *img);
texture_s *create_texture_from_surface(surface_s *sur);

/* page functions */
struct page *create_page(void);
void destroy_page(struct page *page);
void show_page(struct page *page);
void render_showen_page(void);

/* text functions */
struct text *create_text(char *string, int x, int y, int w, int h, int font_size, int wrap_length, color  fg_color, TTF_Font *f, struct page *p);
void destroy_text_texture(struct text *text);
void destroy_text(struct text *text);
void render_text(struct text *t, rect_s *src);
int render_text_texture(struct text *t, color  fg_color, TTF_Font *f);
void change_text_and_render_texture(struct text *text, char *new_text, color  fg_color, TTF_Font *f);

/* button functions */
struct button *create_button(char *string, int movable, int show, int x, int y, int w, int h, int font_size, TTF_Font *f, void (*on_click)(struct button *s, Event *e), void (*on_move)(struct button *b, Event *e), color  outer_color, color  bg_color, color  text_color, struct page *p);
void destroy_button(struct button *button);
void render_button(struct button *button);

/* slider functions */
struct slider *create_slider(int show, int x, int y, int w, int h, int button_size, void (*on_relase)(struct button *s, Event *e), void (*on_move)(struct slider *s, Event *e), color  button_fg_color, color  button_bg_color, color  bar_color, struct page *p);
void render_slider(struct slider *slider);
void update_slider(struct slider *slider, int x);
void destroy_slider(struct slider *slider);

/* input functions */
struct input *create_input(char *text, char *def_text, int resize_box, int max_len, int x, int y, int w, int h, void (*function)(struct input *self, Event *event), TTF_Font *f, color  outer_color, color  bg_color, color  text_color, struct page *p);
void destroy_input_box(struct input *input_box);
void change_input_box_text(struct input *input_box, char *str);
void render_input_box(struct input *input_box);

/* drop down menu functions */
struct drop_down_menu *create_drop_down_menu(int items, char item_str[][MAX_TEXT_SIZE], int x, int y, int w, int h, int dw, int dh, void (*function)(struct drop_down_menu *s, Event *e), TTF_Font *f, color  outer_color, color  bg_color, color  tc, struct page *p);
void destroy_ddm(struct drop_down_menu *ddm);
void remove_item_ddm(struct drop_down_menu *ddm, int item);
void add_item_ddm(struct drop_down_menu *ddm, char *newstr, TTF_Font *f);
void render_ddm(struct drop_down_menu *ddm);
void select_ddm_item(struct drop_down_menu *ddm, int index);
void change_ddm_text_arr(struct drop_down_menu *ddm, int items, char newstr[][MAX_TEXT_SIZE], TTF_Font *f);
void update_ddm_highlight(int x, int y, struct drop_down_menu *ddm);

/* graph functions */
struct graph *create_graph(int x, int y, int w, int h, int scalew, int scaleh, int pw, int ph, int spw, int sph, int pamount, void (*on_move)(struct graph *self, Event *e), color  oc, color  bgc, color  fgc, color  pointc, color  spc, struct page *p);
void destroy_graph(struct graph *graph);
void render_graph(struct graph *graph);
void change_graph_point(struct graph *graph);
int change_graph_points(struct graph *g, struct point *new_points, int new_size);
int copy_points(struct point *points, int *total_size, int *size, struct point *new_points, int new_size);

/* line functions */
struct line *create_line(int x1, int y1, int x2, int y2, color  color, struct page *p);
void destroy_line(struct line *line);
void render_line(struct line *line);

#endif
