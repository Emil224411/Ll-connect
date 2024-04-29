#ifndef UI_H
#define UI_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#define WINDOW_W 800
#define WINDOW_H 400

#define MAX_TEXT_SIZE 256

#define SDL_COLOR_ARG(C) C.r, C.g, C.b, C.a

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

struct text {
	char str[MAX_TEXT_SIZE];
	int show;
	SDL_Color fg_color, bg_color;
	SDL_Rect src, dst;
	SDL_Texture *texture;
} text;

struct button {
	struct text text;
	void (*function) ();
	SDL_Rect outer_box;
	SDL_Color outer_box_color, bg_color;
} button;
static int button_arr_total_len, button_arr_used_len;
static struct button **button_arr;

struct input {
	int index, selected;
	struct text text;
	int resize_box;
	SDL_Rect outer_box;
	SDL_Color outer_box_color, bg_color;

} input;
static struct input *selected_input;
static int input_box_arr_total_len;
static int input_box_arr_used_len;
static struct input **input_box_arr;

int ui_init();
void ui_shutdown();
void handle_event(SDL_Event *event);
void mousebutton(SDL_MouseButtonEvent mouse_data);
void show_screen();
void clear_screen();

struct text create_text(char *string, int x, int y, SDL_Color fg_color, SDL_Color bg_color, TTF_Font* f);
void destroy_text_texture(struct text *text);
void render_text(struct text *t, SDL_Rect *src);
void render_text_texture(struct text *t, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f);
void change_text_and_render_texture(struct text *text, char *new_text, SDL_Color fg_color, SDL_Color bg_color, TTF_Font *f);

struct button *create_button(char *string, int x, int y, int w, int h, TTF_Font *f, void (*function)(), SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color);
void render_button(struct button *button);

struct input *create_input_from_text(struct text text, int resize_box, TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color);
struct input *create_input(char *text, int resize_box, int x, int y, int w, int h, TTF_Font *f, SDL_Color outer_color, SDL_Color bg_color, SDL_Color text_color);
void destroy_input_box(struct input *input_box);
void change_input_box_text(struct input *input_box, char *str);
void render_input_box(struct input *input_box);

#endif
