#ifndef UI_H
#define UI_H
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

SDL_Color BLACK   = {   0,   0,   0, SDL_ALPHA_OPAQUE };
SDL_Color WHITE   = { 255, 255, 255, SDL_ALPHA_OPAQUE };
SDL_Color RED     = { 255,   0,   0, SDL_ALPHA_OPAQUE };
SDL_Color GREEN   = {   0, 255,   0, SDL_ALPHA_OPAQUE };
SDL_Color BLUE    = {   0,   0, 255, SDL_ALPHA_OPAQUE };
SDL_Color YELLOW  = { 255, 255,   0, SDL_ALPHA_OPAQUE };

char font_path[128];
TTF_Font *font;

int running;
SDL_Window   *window;
SDL_Renderer *renderer;
SDL_Event event;

struct text {
	char *str;
	int show, rerender;
	SDL_Rect pos;
	SDL_Texture *texture;
} text;

struct input {
	int selected;
	struct text text;
	SDL_Rect outer_box;
	SDL_Color outer_box_color, background_color, text_color;

} input;

int ui_init();
void ui_shutdown();
void handle_event(SDL_Event *event);
void mouseclick(SDL_MouseButtonEvent *me);
void clear_screen();

struct text create_text(char *t, int x, int y, SDL_Color c, TTF_Font* f);
void destroy_text_texture(struct text *text);
void render_text(struct text *t);
void render_text_texture(struct text *t, SDL_Color color, TTF_Font *f);
void change_text_and_render_texture(struct text *text, char *new_text, SDL_Color color, TTF_Font *f);

struct input create_input(struct text text, TTF_Font *f, SDL_Color outer_color, SDL_Color background_color, SDL_Color text_color);
void render_input_box(struct input *input_box);

#endif
