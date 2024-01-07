#include "../include/ui.h"

int ui_init() 
{
	window = NULL;
	renderer = NULL;
	if (SDL_Init(SDL_INIT_EVERYTHING)) {
		printf("SDL_Init failed err: %s\n", SDL_GetError());
		return 1;
	}

	if (TTF_Init()) {
		printf("TTF_Init failed err: %s\n", TTF_GetError());
		return 1;
	}
	font = TTF_OpenFont("/usr/share/fonts/TTF/HackNerdFont-Regular.ttf", 20);
	window = SDL_CreateWindow("lianlipoop", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 400, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		printf("SDL_CreateWindow failed err: %s\n", SDL_GetError());
		return 1;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		printf("SDL_CreateRenderer failed err: %s\n", SDL_GetError());
		return 1;
	}

	return 0;
	
}

void ui_shutdown()
{
	if (window != NULL) {
		SDL_DestroyWindow(window);
		window = NULL;
	}
	if (renderer != NULL) {
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}
	TTF_Quit();
	SDL_Quit();
}

void destroyPage(page* p)
{
	if (p->buttonArrayLen > 0) {
		free(p->buttonArray);
	}
	if (p->textArrayLen > 0) {
		for (int i = 0; i < p->textArrayLen; i++) SDL_DestroyTexture(p->textArray[i].texture);
		free(p->textArray);
	}
}

void clearscreen()
{
	SDL_SetRenderDrawColor(renderer, black.r, black.g, black.b, black.a);
	SDL_RenderClear(renderer);
}

button *createButton(SDL_Rect pos, text *txt, const SDL_Color *c, void (*funtion)(), page *Page)
{
	button newButton = { pos, txt, 1, c, funtion };
	Page->buttonArrayLen++;
	button *tmpButtonArray = (button *)realloc(Page->buttonArray, sizeof(button) * Page->buttonArrayLen);
	Page->buttonArray = tmpButtonArray;
	Page->buttonArray[Page->buttonArrayLen - 1] = newButton;
	return &Page->buttonArray[Page->buttonArrayLen -1];
}

text *createText(char *t, SDL_Rect *p, const SDL_Color *c, page *Page, TTF_Font* f)
{
	text newText = { t, 1 };
	SDL_Surface *s = TTF_RenderText_Solid(f, t, *c);
	newText.texture = SDL_CreateTextureFromSurface(renderer, s);
	TTF_SizeText(f, t, &p->w, &p->h);
	SDL_FreeSurface(s);
	Page->textArrayLen++;
	text *tmpTextArray = (text *)realloc(Page->textArray, sizeof(text) * Page->textArrayLen);
	Page->textArray = tmpTextArray;
	Page->textArray[Page->textArrayLen-1].pos = (*p);
	Page->textArray[Page->textArrayLen-1].show = newText.show;
	Page->textArray[Page->textArrayLen-1].str = t;
	Page->textArray[Page->textArrayLen-1].texture = newText.texture;
	return &Page->textArray[Page->textArrayLen-1];
}

void changeText(text *t, char *nt, SDL_Rect *p, const SDL_Color *c, TTF_Font *f)
{
	t->str = nt;
	SDL_Surface *s = TTF_RenderText_Solid(f, nt, *c);
	t->texture = SDL_CreateTextureFromSurface(renderer, s);
	TTF_SizeText(f, nt, &p->w, &p->h);
	SDL_FreeSurface(s);
	t->pos = (*p);
}

void renderText(text *t)
{
	SDL_RenderCopy(renderer, t->texture, NULL, &t->pos);
}

void mouseclick(SDL_MouseButtonEvent *me)
{
	if (me->button == SDL_BUTTON_LEFT) {
		for (int i = 0; i < currentPage->buttonArrayLen; i++) {
			if (currentPage->buttonArray[i].funtion){
				if (me->x > currentPage->buttonArray[i].pos.x && me->x < currentPage->buttonArray[i].pos.x + currentPage->buttonArray[i].pos.w 
					&& me->y > currentPage->buttonArray[i].pos.y && me->y < currentPage->buttonArray[i].pos.y + currentPage->buttonArray[i].pos.h) {
					currentPage->buttonArray[i].funtion();
				}
			}
		}
	}
}

void hadleEvent(SDL_Event *e)
{
	switch (e->type) {
		case SDL_QUIT:
			running = 0;
			break;
		case SDL_MOUSEBUTTONDOWN:
			mouseclick(&e->button);
			break;
		case SDL_KEYDOWN:
			switch (e->key.keysym.scancode) {
				case SDL_SCANCODE_ESCAPE:
					running = 0;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void renderButton(button* b)
{
	SDL_SetRenderDrawColor(renderer, b->color->r, b->color->g, b->color->b, b->color->a);
	SDL_RenderDrawRect(renderer, &b->pos);
}

void renderPage(page *p)
{
	for (int i = 0; i < p->buttonArrayLen; i++) {
		if (p->buttonArray[i].show) {
			renderButton(&p->buttonArray[i]);
		}
	}
	for (int i = 0; i < p->textArrayLen; i++) {
		if (p->textArray[i].show) {
			renderText(&p->textArray[i]);
		}
	}
}

/*void mainloop(text *cput, SDL_Rect *cpupos)
{
	running = 1;
	SDL_Event event;
	unsigned int a = SDL_GetTicks();
	unsigned int b = SDL_GetTicks();
	unsigned int b2 = SDL_GetTicks();
	double delta = 0;
	double delta2 = 0;

	while (running) {
		a = SDL_GetTicks();
		delta = a - b;
		delta2 = a - b2;

		if (SDL_PollEvent(&event)) {
			hadleEvent(&event);
		}
		if (delta2 > 2000) {
			b2 = a;
			updatetemp(cput, cpupos);
		}
		if (delta > 1000/60.0) {
			b = a;
			clearscreen();
			
			renderPage(currentPage);

			SDL_RenderPresent(renderer);
		}
	}
}*/
