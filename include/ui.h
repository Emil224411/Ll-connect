#ifndef UI_H
#define UI_H
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

const SDL_Color black   = {   0,   0,   0, SDL_ALPHA_OPAQUE };
const SDL_Color white 	= { 255, 255, 255, SDL_ALPHA_OPAQUE };
const SDL_Color red     = { 255,   0,   0, SDL_ALPHA_OPAQUE };
const SDL_Color green   = {   0, 255,   0, SDL_ALPHA_OPAQUE };
const SDL_Color blue    = {   0,   0, 255, SDL_ALPHA_OPAQUE };
const SDL_Color yellow  = { 255, 255,   0, SDL_ALPHA_OPAQUE };

TTF_Font *font;

int running;
SDL_Window 	 *window;
SDL_Renderer *renderer;
SDL_Event event;

typedef struct text {
	char *str;
	int show;
	SDL_Rect pos;
	SDL_Texture* texture;
} text;
int textArrayLen = 0;
text *textArray;

typedef struct button {
	SDL_Rect pos;
	text *txt;
	int show;
	const SDL_Color *color;
	void (*funtion)();
} button;
int buttonArrayLen = 0;
button *buttonArray;

typedef struct page {
	int textArrayLen, buttonArrayLen;
	text *textArray;
	button *buttonArray;
	SDL_Color pageColor;
} page;
page *currentPage;

int ui_init();
//void mainloop(text *cput, SDL_Rect *cpupos);
//void updatetemp(text *cput, SDL_Rect *pos);
text *createText(char *t, SDL_Rect *p, const SDL_Color *c, page *Page, TTF_Font* f);
void changeText(text *t, char *nt, SDL_Rect *p, const SDL_Color *c, TTF_Font *f);
button *createButton(SDL_Rect pos, text *txt, const SDL_Color *c, void (*funtion)(), page *Page);
void renderText(text *t);
void renderButton(button *b);
void renderPage(page *p);
void destroyPage(page *p);
void hadleEvent(SDL_Event *e);
void mouseclick(SDL_MouseButtonEvent *me);
void clearscreen();
void ui_shutdown();

#endif
