#ifndef UI_H
#define UI_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

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

struct text {
	char str[MAX_TEXT_SIZE];
	int show, static_w, static_h, wrap_length;
	SDL_Color fg_color, bg_color;
	SDL_Rect src, dst;
	SDL_Texture *texture;
};

struct button {
	struct text text;
	void (*on_click) (struct button *self, SDL_Event *event);
	void (*on_move) (struct button *self, SDL_Event *event);
	int movable, show;
	SDL_Rect outer_box;
	SDL_Color outer_box_color, bg_color;
};
static struct button *selected_button;
static int button_arr_total_len, button_arr_used_len;
static struct button **button_arr;

struct input {
	int index, selected, max_len;
	struct text text;
	int resize_box;
	void (*function)(struct input *self, SDL_Event *event);
	SDL_Rect default_outer_box, outer_box;
	SDL_Color outer_box_color, bg_color;

};
static struct input *selected_input;
static int input_box_arr_total_len;
static int input_box_arr_used_len;
static struct input **input_box_arr;

struct drop_down_menu {
	int selected, selected_text_index, update_highlight;
	int index, static_w, static_h; /* privat */
	int items;
	struct text *text;
	void (*function)(struct drop_down_menu *self, SDL_Event *event);
	int scroll_offset;
	SDL_Rect default_pos, used_pos, drop_pos, highlight_pos;
	SDL_Color outer_box_color, bg_color;
};
static struct drop_down_menu **ddm_arr;
static struct drop_down_menu *selected_ddm;
static int ddm_arr_total_len;
static int ddm_arr_used_len;


int ui_init();
void ui_shutdown();
void handle_event(SDL_Event *event);
void lmouse_button_down(SDL_Event *event);
void lmouse_button_up(SDL_Event *event);
void mouse_wheel(SDL_MouseWheelEvent *event);
void mouse_move(SDL_Event *event);
void show_screen();
void clear_screen(SDL_Color color);

SDL_Texture *create_texture_from_surface(SDL_Surface *sur);

struct text create_text(char *string, int x, int y, int w, int h, int wrap_length, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f);
void destroy_text_texture(struct text *text);
void render_text(struct text *t, SDL_Rect *src);
int render_text_texture(struct text *t, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f);
void change_text_and_render_texture(struct text *text, char *new_text, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f);

struct button *create_button(char *string, int movable, int show, int x, int y, int w, int h, TTF_Font *f, void (*on_click)(), void (*on_move)(), SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color);
void render_button(struct button *button);

struct input *create_input_from_text(struct text text, int resize_box, void (*function)(struct input *self, SDL_Event *event), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color);
struct input *create_input(char *text, int resize_box, int max_len, int x, int y, int w, int h, void (*function)(struct input *self, SDL_Event *event), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color);
void destroy_input_box(struct input *input_box);
void change_input_box_text(struct input *input_box, char *str);
void render_input_box(struct input *input_box);

struct drop_down_menu *create_drop_down_menu(int items, char item_str[][MAX_TEXT_SIZE], int x, int y, int w, int h, int dw, int dh, void (*function)(struct drop_down_menu *self, SDL_Event *event), TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color tc);
void destroy_ddm(struct drop_down_menu *ddm);
void render_ddm(struct drop_down_menu *ddm);
void change_ddm_text_arr(struct drop_down_menu *ddm, int items, char newstr[][MAX_TEXT_SIZE], TTF_Font *f);
void update_ddm_highlight(int x, int y, struct drop_down_menu *ddm);

#endif
