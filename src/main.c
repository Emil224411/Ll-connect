#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>

#include "../include/ui.h"

/*
void updatetemp(text *cput, SDL_Rect *pos) {
	FILE *fcpu = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	int icputemp;
	fscanf(fcpu, "%d", &icputemp);
	float cputemp = icputemp/1000.0;
	char str[10];
	sprintf(str, "%.2f", cputemp); 
	changeText(cput, str, pos, &white, font);
	fclose(fcpu);
}
*/
SDL_Surface *color_picker_surface;
SDL_Rect color_picker_pos = { 100, 50, };
SDL_Color grey = { 209, 209, 209, SDL_ALPHA_OPAQUE };
SDL_Color darkgrey = { 84, 84, 84, SDL_ALPHA_OPAQUE };

inline SDL_Surface *create_color_picker_surface(int w, int h, int depth);
void select_button(struct button *self, SDL_Event *event);
void deselect_input(struct input *self, SDL_Event *event);
int init();

FILE *fan_speeds;
char speeds_str[MAX_TEXT_SIZE];
struct text speeds;
int speeds_pro[4] = { 0 };
int speeds_rpm[4] = { 0 };

SDL_Texture *color_picker_texture;
struct button *color_picker_selector;
SDL_Color buttoncolor = { 0xff, 0, 0, 0xff};
struct input *color_input;
struct text color_text;

int main()
{
	if (init() != 0) {
		printf("init failed:(\n");
		return 1;
	}
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

		while (SDL_PollEvent(&event)) {
			handle_event(&event);
		}

		if (delta > 1000/60.0) {
			b = a;
			clear_screen();

			SDL_RenderCopy(renderer, color_picker_texture, NULL, &color_picker_pos);
			render_button(color_picker_selector);
			render_text(&speeds, NULL);
			render_input_box(color_input);
			//render_text(&color_text, NULL);
			show_screen();
		}
		if (delta2 > 1000/1.0) {
			b2 = a;
			fan_speeds = fopen("/proc/fan_speeds", "r+");
			if (fan_speeds == NULL) {
				printf("fan_speeds failed to open\n");
			} else {
				fscanf(fan_speeds, "port\t:\t1,\t2,\t3,\t4\n%%\t:\t%d,\t%d,\t%d,\t%d\nrpm\t:\t%d,\t%d,\t%d,\t%d\n", 
						&speeds_pro[0], &speeds_pro[1], &speeds_pro[2], &speeds_pro[3],
						&speeds_rpm[0], &speeds_rpm[1], &speeds_rpm[2], &speeds_rpm[3]);
				sprintf(speeds_str, "%d, %d, %d, %d\n%d, %d, %d, %d", 
						speeds_pro[0], speeds_pro[1], speeds_pro[2], speeds_pro[3], 
						speeds_rpm[0], speeds_rpm[1], speeds_rpm[2], speeds_rpm[3]);

				change_text_and_render_texture(&speeds, speeds_str, speeds.fg_color, speeds.bg_color, font);
				fclose(fan_speeds);
			}
		}
		SDL_Delay(1);
	}

	SDL_DestroyTexture(color_picker_texture);
	SDL_FreeSurface(color_picker_surface);
	destroy_text_texture(&speeds);
	ui_shutdown();

	return 0;
}
int init()
{
	strcpy(font_path, "/usr/share/fonts/TTF/HackNerdFont-Regular.ttf");
	if (ui_init()) {
		printf("ui_init failed\n");
		return 1;
	}
	running = 1;
	FILE *fan_speeds = fopen("/proc/fan_speeds", "r+");
	if (fan_speeds == NULL) {
		printf("file not found:(\n");
		ui_shutdown();
		return 1;
	}
	fscanf(fan_speeds, "port\t:\t1,\t2,\t3,\t4\n%%\t:\t%d,\t%d,\t%d,\t%d\nrpm\t:\t%d,\t%d,\t%d,\t%d\n", 
					&speeds_pro[0], &speeds_pro[1], &speeds_pro[2], &speeds_pro[3],
					&speeds_rpm[0], &speeds_rpm[1], &speeds_rpm[2], &speeds_rpm[3]);
	fclose(fan_speeds);

	sprintf(speeds_str, "%d, %d, %d, %d\n%d, %d, %d,%d", speeds_pro[0], speeds_pro[1], speeds_pro[2], speeds_pro[3], speeds_rpm[0], speeds_rpm[1], speeds_rpm[2], speeds_rpm[3]);
	speeds = create_text(speeds_str, 10, 10, 0, 0, 0, WHITE, BLACK, font);

	SDL_Color buttoncolor = { 0xff, 0, 0, 0xff};
	color_picker_selector = create_button(NULL, 1, WINDOW_W-200, WINDOW_H-200, 25, 25, font, select_button, WHITE, buttoncolor, WHITE);

	color_picker_surface = create_color_picker_surface(200, 200, 32);
	color_picker_pos.x = WINDOW_W-250;
	color_picker_pos.y = WINDOW_H-250;
	color_picker_pos.w = color_picker_pos.h = 200;
	
	color_picker_texture = create_texture_from_surface(color_picker_surface);

	//color_text = create_text("100", color_picker_pos.x, color_picker_pos.y + color_picker_pos.w + 10, 0, 0, 0, WHITE, BLACK, font);
	color_input = create_input("255, 255, 255", 0, color_picker_pos.x, color_picker_pos.y + color_picker_pos.h, 0, 0, deselect_input, font, WHITE, BLACK, WHITE);

	return 0;
}
SDL_Surface *create_color_picker_surface(int w, int h, int depth)
{
	SDL_Surface *return_surface = SDL_CreateRGBSurface(0, w, h, depth, 0, 0, 0, 0);
	Uint32 *rgb = (Uint32 *)return_surface->pixels;
	Uint8 red, green, blue;

	float colorthing = return_surface->w/6;
	float number = 1530.1f / return_surface->w;

	for (int y = 0; y < return_surface->h; y++) {
		red =   0xff;
		green = 0;
		blue =  0;
		for(int i = 0; i < colorthing; i++) {
			green = i * number;
			rgb[y * return_surface->w + i + (int)(colorthing * 0)] = 0xff000000 + (red << 16) + (green << 8) + blue;
		}
		green = 0xff;
		int x = colorthing;
		for(int i = 0; i < colorthing; i++) {
			x--;
			red = x * number;
			rgb[y * return_surface->w + i + (int)(colorthing * 1)] = 0xff000000 + (red << 16) + (green << 8) + blue;
		}
		
		for(int i = 0; i < colorthing; i++) {
			blue = i * number;
			rgb[y * return_surface->w + i + (int)(colorthing*2)] = 0xff000000 + (red << 16) + (green << 8) + blue;
		}
		blue = 0xff;
		x = colorthing;
		for(int i = 0; i < colorthing; i++) {
			x--;
			green = x * number;
			rgb[y * return_surface->w + i + (int)(colorthing*3)] = 0xff000000 + (red << 16) + (green << 8) + blue;
		}
		for (int i = 0; i < colorthing; i++) {
			red = i * number;
			rgb[y * return_surface->w + i + (int)(colorthing*4)] = 0xff000000 + (red << 16) + (green << 8) + blue;
		}
		red = 0xff;
		x = colorthing;
		for (int i = 0; i < colorthing; i++) {
			x--;
			blue = x * number;
			rgb[y * return_surface->w + i + (int)(colorthing*5)] = 0xff000000 + (red << 16) + (green << 8) + blue;
		}
	}
	printf("%08x\n", rgb[return_surface->w]);
	printf("%08x\n", return_surface->w);
	printf("%08x\n", rgb[255 * return_surface->w / (255+255+255)]);
	printf("%08x\n", rgb[0]);
	return return_surface;
}

/*
// TODO: index into surface pixels with input_box text 
// 0 to 0xc8 
// so from 255, 255, 255 to 0xc8
// 	   |	   |
// 	  \ /input\ /
// mabey pixels[(100 + 200 + 0) * color_picker_surface->w / (255 + 255 + 255)]
// dont work:( but figure something out 
// btw create_color_picker_surface its not great
//
*/
void deselect_input(struct input *self, SDL_Event *event)
{
//	color_picker_pos.x = 
}

void select_button(struct button *self, SDL_Event *event)
{
	SDL_MouseMotionEvent mouse_data = event->motion;

	self->outer_box.x = mouse_data.x; 
	self->outer_box.y = mouse_data.y; 

	if (self->outer_box.x > color_picker_pos.x + color_picker_pos.w-1) {
		self->outer_box.x = color_picker_pos.x + color_picker_pos.w - 1;
	} 
	else if (self->outer_box.x < color_picker_pos.x) {
		self->outer_box.x = color_picker_pos.x;
	} 

	if (self->outer_box.y > color_picker_pos.y + color_picker_pos.h-1) {
		self->outer_box.y = color_picker_pos.y + color_picker_pos.h-1;
	}
	else if (self->outer_box.y < color_picker_pos.y) {
		self->outer_box.y = color_picker_pos.y;
	} 

	Uint32 *pixels = (Uint32*)color_picker_surface->pixels;
	Uint32 pixel = pixels[(self->outer_box.y - color_picker_pos.y) * 
					color_picker_surface->w + (self->outer_box.x - color_picker_pos.x)];

	self->bg_color.r = pixel >> 16;
	self->bg_color.g = pixel >> 8;
	self->bg_color.b = pixel;
	self->outer_box.x -= self->outer_box.w/2;
	self->outer_box.y -= self->outer_box.h/2;

	char new_text[MAX_TEXT_SIZE];
	sprintf(new_text, "%d, %d, %d", self->bg_color.r, self->bg_color.g, self->bg_color.b);
	change_input_box_text(color_input, new_text);
	//change_text_and_render_texture(&color_text, new_text, WHITE, BLACK, font);
}





























