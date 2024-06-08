#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <SDL2/SDL.h>

#include "../include/ui.h"
#include "../include/controller.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

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

SDL_Color grey = { 209, 209, 209, SDL_ALPHA_OPAQUE };
SDL_Color darkgrey = { 84, 84, 84, SDL_ALPHA_OPAQUE };

SDL_Surface *other_picker_surface;
SDL_Rect other_picker_pos;

SDL_Surface *color_picker_surface;
SDL_Rect color_picker_pos;


inline SDL_Surface *create_rgb_color_picker_surface();
inline SDL_Surface *create_white_black_picker(Uint8 red, Uint8 green, Uint8 blue);
void select_button(struct button *self, SDL_Event *event);
void select_button2(struct button *self, SDL_Event *event);
void deselect_input(struct input *self, SDL_Event *event);
void ddm_select(struct drop_down_menu *d, SDL_Event *event);
int update_speed_str();
int init();

char speeds_str[MAX_TEXT_SIZE];
struct text speeds;
int speeds_pro[4] = { 0 };
int speeds_rpm[4] = { 0 };

SDL_Texture *other_picker_texture;
SDL_Texture *color_picker_texture;
struct button *color_selector;
struct button *black_white_selector;
struct input *color_input;
struct text color_text;

struct button *color_buttons[6];
struct drop_down_menu *ddm;
int rgb_mode_i;

int main()
{
	if (init() != 0) {
		printf("init failed:(\n");
		return 1;
	}
	struct color new_outer_color[48] = { 0 };
	for (int i = 0; i < 48; i++) {
		new_outer_color[i].r = 0xff;
		new_outer_color[i].g = 0;
		new_outer_color[i].b = 0;
	}
	set_outer_rgb(&ports[1], &rgb_modes[0], 0x02, 0x0, 0x0, new_outer_color);

	SDL_Event event;
	unsigned int a = SDL_GetTicks();
	unsigned int b = SDL_GetTicks();
	unsigned int b2 = SDL_GetTicks();
	double delta = 0;
	double delta2 = 0;
	
	running = 1;
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
			SDL_RenderCopy(renderer, other_picker_texture, NULL, &other_picker_pos);
			render_button(color_selector);
			render_button(black_white_selector);
			render_text(&speeds, NULL);
			render_input_box(color_input);
			
			render_ddm(ddm);
			show_screen();
		}
		if (delta2 > 1000/1.0) {
			b2 = a;
			update_speed_str();		
			change_text_and_render_texture(&speeds, speeds_str, speeds.fg_color, speeds.bg_color, font);
		}
		SDL_Delay(1);
	}

	SDL_DestroyTexture(color_picker_texture);
	SDL_DestroyTexture(other_picker_texture);
	SDL_FreeSurface(color_picker_surface);
	SDL_FreeSurface(other_picker_surface);
	destroy_text_texture(&speeds);
	ui_shutdown();

	return 0;
}

int init()
{
	
	FILE *f = fopen("/proc/modules", "r");
	char *line;
	size_t n;
	int mod_loaded = 0;
	while (getline(&line, &n, f) != -1) {
		if (strncmp("Lian_li_hub", line, strlen("Lian_li_hub")) == 0) {
			mod_loaded = 1;
		}
	}
	if (!mod_loaded) {
		printf("load kernel module Lian_li_hub and try again\n");
		free(line);
		fclose(f);
		return -1;
	}
	strcpy(font_path, "/usr/share/fonts/TTF/HackNerdFont-Regular.ttf");
	if (ui_init()) {
		printf("ui_init failed\n");
		return -1;
	}

	if (update_speed_str() != 0) {
		printf("update_speed_str failed");
		return -1;
	}

	speeds = create_text(speeds_str, 10, 10, 0, 0, 0, WHITE, BLACK, font);

	color_picker_pos.x = WINDOW_W-300;
	color_picker_pos.y = WINDOW_H-100;
	color_picker_pos.w = 200;
	color_picker_pos.h = 20;

	color_picker_surface = create_rgb_color_picker_surface();
	color_picker_texture = create_texture_from_surface(color_picker_surface);

	color_input = create_input("255, 255, 255", 0, color_picker_pos.x, color_picker_pos.y + color_picker_pos.h + 10, 0, 0, NULL, font, WHITE, BLACK, WHITE);

	other_picker_pos.x = color_picker_pos.x;
	other_picker_pos.y = color_picker_pos.y - 210;
	other_picker_pos.w = other_picker_pos.h = 200;
	other_picker_surface = create_white_black_picker(255, 0, 0);
	other_picker_texture = create_texture_from_surface(other_picker_surface);

	color_selector = create_button(NULL, 1, color_picker_pos.x, color_picker_pos.y, 15, 15, font, select_button2, WHITE, BLACK, WHITE);
	black_white_selector = create_button(NULL, 1, other_picker_pos.x, other_picker_pos.y + other_picker_pos.h/2, 25, 25, font, select_button, WHITE, BLACK, WHITE);
	
	for (int i = 0; i < 6; i++) {
		color_buttons[i] = create_button(NULL, 0, WINDOW_W-350, WINDOW_H-100, 50, 50, font, NULL, WHITE, BLACK, WHITE);
	}
	char tmp_str[30][256];
	for (int i = 0; i < 30; i++) {
		strncpy(tmp_str[i], rgb_modes[i].name, MAX_TEXT_SIZE);
		tmp_str[i][255] = '\0';
	}
	
	ddm = create_drop_down_menu(30, tmp_str, 10, 100, 150, 30, 150, 300, ddm_select, font, WHITE, BLACK, WHITE);

	return 0;
}

void ddm_select(struct drop_down_menu *d, SDL_Event *event)
{
	if (d->selected) {
		rgb_mode_i = d->selected_text_index;
		printf("rgb_mode_i = %d, rgb_mode = %s\n", rgb_mode_i, rgb_modes[rgb_mode_i].name);
	}
}


int update_speed_str()
{
	char path[MAX_TEXT_SIZE];

	strcpy(path, PORT_ONE_PATH);
	strcat(path, "/fan_speed");
	FILE *f = fopen(path, "r+");
	if (f == NULL) {
		printf("fan_speeds Port_one failed to open\n");
		return -1;
	}
	fscanf(f, "%d %d\n", &speeds_pro[0], &speeds_rpm[0]);
	fclose(f);

	strcpy(path, PORT_TWO_PATH);
	strcat(path, "/fan_speed");
	f = fopen(path, "r+");
	if (f == NULL) {
		printf("fan_speeds Port_two failed to open\n");
		return -1;
	}
	fscanf(f, "%d %d\n", &speeds_pro[1], &speeds_rpm[1]);
	fclose(f);

	strcpy(path, PORT_THREE_PATH);
	strcat(path, "/fan_speed");
	f = fopen(path, "r+");
	if (f == NULL) {
		printf("fan_speeds Port_three failed to open\n");
		return -1;
	}
	fscanf(f, "%d %d\n", &speeds_pro[2], &speeds_rpm[2]);
	fclose(f);

	strcpy(path, PORT_FOUR_PATH);
	strcat(path, "/fan_speed");
	f = fopen(path, "r+");
	if (f == NULL) {
		printf("fan_speeds Port_four failed to open\n");
		return -1;
	}
	fscanf(f, "%d %d\n", &speeds_pro[3], &speeds_rpm[3]);

	sprintf(speeds_str, "%d, %d, %d, %d\n%d, %d, %d, %d", 
			speeds_pro[0], speeds_pro[1], speeds_pro[2], speeds_pro[3], 
			speeds_rpm[0], speeds_rpm[1], speeds_rpm[2], speeds_rpm[3]);

	fclose(f);
	return 0;
}

void create_colors()
{

}

SDL_Surface *create_rgb_color_picker_surface()
{
	SDL_Surface *return_surface = SDL_CreateRGBSurface(0, 1530, 1, 32, 0, 0, 0, 0);
	Uint32 *rgb = (Uint32 *)return_surface->pixels;
	Uint8 red, green, blue;

	//float colorthing = return_surface->w/6;
	float colorthing = 1530/6;

	red =   0xff;
	green = 0;
	blue =  0;
	for(int i = 0; i < colorthing; i++) {
		green = i;
		rgb[/*y * return_surface->w + */i + (int)(colorthing * 0)] = 0xff000000 + (red << 16) + (green << 8) + blue;
	}
	green = 0xff;
	int x = colorthing;
	for(int i = 0; i < colorthing; i++) {
		x--;
		red = x;
		rgb[i + (int)(colorthing * 1)] = 0xff000000 + (red << 16) + (green << 8) + blue;
	}
	
	for(int i = 0; i < colorthing; i++) {
		blue = i;
		rgb[i + (int)(colorthing*2)] = 0xff000000 + (red << 16) + (green << 8) + blue;
	}
	blue = 0xff;
	x = colorthing;
	for(int i = 0; i < colorthing; i++) {
		x--;
		green = x;
		rgb[i + (int)(colorthing*3)] = 0xff000000 + (red << 16) + (green << 8) + blue;
	}
	for (int i = 0; i < colorthing; i++) {
		red = i;
		rgb[i + (int)(colorthing*4)] = 0xff000000 + (red << 16) + (green << 8) + blue;
	}
	red = 0xff;
	//if you dont do colorthing - 5 then the blue does not go to 0
	x = colorthing - 5;
	for (int i = 0; i < colorthing; i++) {
		x = (int)x == 0 ? 0 : x - 1;
		blue = x;
		rgb[i + (int)(colorthing*5)] = 0xff000000 + (red << 16) + (green << 8) + blue;
	}
	return return_surface;
}

void change_white_black_picker(SDL_Surface *surface, Uint8 red, Uint8 green, Uint8 blue)
{
	Uint32 *pixels = (Uint32 *)surface->pixels;
	float black;
	float redf = (float)red;
	float greenf = (float)green;
	float bluef = (float)blue;
	for (int i = 0; i < surface->h; i++) {
		black = (float)i / (surface->h);
		float newredf   = redf * black;
		float newgreenf = greenf * black;
		float newbluef  = bluef * black;
		for (int j = 0; j < surface->w; j++) {
			Uint8 newred = newredf + j * ((255-redf)/surface->w) * black;
			Uint8 newgreen = newgreenf + j * ((255-greenf)/surface->w) * black;
			Uint8 newblue = newbluef + j * ((255-bluef)/surface->w) * black;

			pixels[i * surface->w + j] = 0xff000000 + (newred << 16) + (newgreen << 8) + newblue;
		}
	}
}

SDL_Surface *create_white_black_picker(Uint8 red, Uint8 green, Uint8 blue)
{
	SDL_Surface *return_surface = SDL_CreateRGBSurface(0, 256, 256, 32, 0, 0, 0, 0);
	Uint32 *pixels = (Uint32 *)return_surface->pixels;
	float black;
	float redf = (float)red;
	float greenf = (float)green;
	float bluef = (float)blue;
	for (int i = 0; i < return_surface->h; i++) {
		black = (float)i / (return_surface->h-1);
		float newredf   = redf * black;
		float newgreenf = greenf * black;
		float newbluef  = bluef * black;
		for (int j = 0; j < return_surface->w; j++) {
			float whiter = newredf + j * ((255-redf)/return_surface->w) * black;
			float whiteg = newgreenf + j * ((255-greenf)/return_surface->w) * black;
			float whiteb = newbluef + j * ((255-bluef)/return_surface->w) * black;

			Uint8 newred   = whiter; 
			Uint8 newgreen = whiteg;
			Uint8 newblue  = whiteb;
			pixels[i * return_surface->w + j] = 0xff000000 + (newred << 16) + (newgreen << 8) + newblue;
		}
	}
	return return_surface;
}

void deselect_input(struct input *self, SDL_Event *event)
{
//	color_picker_pos.x = 
}

void select_button2(struct button *self, SDL_Event *event)
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
	//Uint32 pixel = pixels[(self->outer_box.y - color_picker_pos.y) * 
	//				color_picker_surface->w + (self->outer_box.x - color_picker_pos.x)];

	Uint32 pixel = pixels[((self->outer_box.y - color_picker_pos.y)*color_picker_surface->h/color_picker_pos.h) * 
					color_picker_surface->w + ((self->outer_box.x - color_picker_pos.x) * color_picker_surface->w/color_picker_pos.w)];

	self->bg_color.r = pixel >> 16;
	self->bg_color.g = pixel >> 8;
	self->bg_color.b = pixel;
	self->outer_box.x -= self->outer_box.w/2;
	self->outer_box.y -= self->outer_box.h/2;

/*
 * TODO: better rendering stuff
 */
	change_white_black_picker(other_picker_surface, self->bg_color.r, self->bg_color.g, self->bg_color.b);
	other_picker_texture = create_texture_from_surface(other_picker_surface);
	char new_text[MAX_TEXT_SIZE];
	sprintf(new_text, "%d, %d, %d", self->bg_color.r, self->bg_color.g, self->bg_color.b);
	change_input_box_text(color_input, new_text);
	//change_text_and_render_texture(&color_text, new_text, WHITE, BLACK, font);
}

void select_button(struct button *self, SDL_Event *event)
{
	SDL_MouseMotionEvent mouse_data = event->motion;

	self->outer_box.x = mouse_data.x; 
	self->outer_box.y = mouse_data.y; 

	if (self->outer_box.x > other_picker_pos.x + other_picker_pos.w-1) {
		self->outer_box.x = other_picker_pos.x + other_picker_pos.w - 1;
	} 
	else if (self->outer_box.x < other_picker_pos.x) {
		self->outer_box.x = other_picker_pos.x;
	} 

	if (self->outer_box.y > other_picker_pos.y + other_picker_pos.h-1) {
		self->outer_box.y = other_picker_pos.y + other_picker_pos.h-1;
	}
	else if (self->outer_box.y < other_picker_pos.y) {
		self->outer_box.y = other_picker_pos.y;
	} 

	Uint32 *pixels = (Uint32*)other_picker_surface->pixels;
	//Uint32 pixel = pixels[(self->outer_box.y - color_picker_pos.y) * 
	//				color_picker_surface->w + (self->outer_box.x - color_picker_pos.x)];

	Uint32 pixel = pixels[((self->outer_box.y - other_picker_pos.y)*other_picker_surface->h/other_picker_pos.h) * 
					other_picker_surface->w + ((self->outer_box.x - other_picker_pos.x) * other_picker_surface->w/other_picker_pos.w)];

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

























