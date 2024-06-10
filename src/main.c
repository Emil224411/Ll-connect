 /**************************************************************************************************************\
 |														|
 |		currently main.c is mostly just testing stuff but some of it is going to stay			|
 |														|
 |--------------------------------------------------------------------------------------------------------------|
 |														|
 |						TODO list							|
 | 		1. look at the usb packets being sent when setting rgb mode to Rainbow. 			|
 | 		2. fix color pickers so they update the color input_box porperly.				|
 |		3. make the input_box for the colors work as a input_box not just a box:/.			|
 |		4. not very importent rn but refactor or rewrite the input_box code. 				|
 | 														|
 |														|
 \**************************************************************************************************************/
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
 * really old function so it wont work but it will be usefull later
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
SDL_Color edarkgrey = { 23, 23, 23, 255 };

SDL_Surface *other_picker_surface;
SDL_Rect other_picker_pos;

SDL_Surface *color_picker_surface;
SDL_Rect color_picker_pos;


inline SDL_Surface *create_rgb_color_picker_surface();
inline SDL_Surface *create_white_black_picker(Uint8 red, Uint8 green, Uint8 blue);
void change_white_black_picker(SDL_Surface *surface, Uint8 red, Uint8 green, Uint8 blue);
void select_button(struct button *self, SDL_Event *event);
void select_button2(struct button *self, SDL_Event *event);
void deselect_input(struct input *self, SDL_Event *event);
void rgb_mode_ddm_select(struct drop_down_menu *d, SDL_Event *event);
void fan_ring_select(struct drop_down_menu *d, SDL_Event *event);
void color_buttons_click(struct button *self, SDL_Event *e);
void create_color_buttons();
void apply(struct button *self, SDL_Event *e);
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
int change;
struct input *color_input;
struct text color_text;

struct button *color_buttons[6];
int selected_color_button;
struct button *apply_rgb;
struct drop_down_menu *rgb_mode_ddm;
struct drop_down_menu *fan_ring_ddm;
int rgb_mode_i;
int rgb_mode_ring_type;

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
			clear_screen(edarkgrey);

			if (change == 1) {
				change_white_black_picker(other_picker_surface, color_selector->bg_color.r, color_selector->bg_color.g, color_selector->bg_color.b);
				SDL_DestroyTexture(other_picker_texture);
				other_picker_texture = create_texture_from_surface(other_picker_surface);
				change = 0;
			}
			SDL_RenderCopy(renderer, color_picker_texture, NULL, &color_picker_pos);
			SDL_RenderCopy(renderer, other_picker_texture, NULL, &other_picker_pos);
			render_button(color_selector);
			render_button(apply_rgb);
			render_button(black_white_selector);
			for (int i = 0; i < 6; i++) render_button(color_buttons[i]);
			render_text(&speeds, NULL);
			render_input_box(color_input);
			
			render_ddm(rgb_mode_ddm);
			render_ddm(fan_ring_ddm);
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
	if (f == NULL) {
		printf("failed fopen /proc/modules\n");
		return -1;
	}
	char *line = malloc(sizeof(char)*MAX_TEXT_SIZE);
	size_t n;
	int mod_loaded = 0;
	while (getline(&line, &n, f) != -1 && mod_loaded == 0) {
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
	free(line);
	fclose(f);
	strcpy(font_path, "/usr/share/fonts/TTF/HackNerdFont-Regular.ttf");
	if (ui_init()) {
		printf("ui_init failed\n");
		return -1;
	}

	if (update_speed_str() != 0) {
		printf("update_speed_str failed");
		return -1;
	}

	speeds = create_text(speeds_str, 10, 10, 0, 0, 0, WHITE, edarkgrey, font);

	color_picker_pos.x = WINDOW_W-400;
	color_picker_pos.y = WINDOW_H-100;
	color_picker_pos.w = 300;
	color_picker_pos.h = 20;

	color_picker_surface = create_rgb_color_picker_surface();
	color_picker_texture = create_texture_from_surface(color_picker_surface);

	color_input = create_input("255, 255, 255", 0, 13, color_picker_pos.x, color_picker_pos.y + color_picker_pos.h + 10, 0, 0, NULL, font, WHITE, edarkgrey, WHITE);

	other_picker_pos.x = color_picker_pos.x;
	other_picker_pos.y = color_picker_pos.y - 210;
	other_picker_pos.w = 300;
	other_picker_pos.h = 200;
	other_picker_surface = create_white_black_picker(255, 0, 0);
	other_picker_texture = create_texture_from_surface(other_picker_surface);

	color_selector = create_button(NULL, 1, 1, color_picker_pos.x, color_picker_pos.y + (color_picker_pos.h/2-10), 20, 20, font, select_button2, select_button2, WHITE, BLACK, WHITE);
	black_white_selector = create_button(NULL, 1, 1, other_picker_pos.x, other_picker_pos.y + other_picker_pos.h/2, 25, 25, font, select_button, select_button, WHITE, BLACK, WHITE);
	
	create_color_buttons();
	char tmp_str[rgb_modes_amount][MAX_TEXT_SIZE];
	for (int i = 0; i < rgb_modes_amount; i++) {
		strncpy(tmp_str[i], rgb_modes[i].name, MAX_TEXT_SIZE);
	}
	
	rgb_mode_ddm = create_drop_down_menu(rgb_modes_amount, tmp_str, 45, 100, 150, 0, 150, 300, rgb_mode_ddm_select, font, WHITE, edarkgrey, WHITE);
	char str[][MAX_TEXT_SIZE] = { "O", "I", "OI" };
	fan_ring_ddm = create_drop_down_menu(3, str, 10, 100, 0, 0, 0, 70, fan_ring_select, font, WHITE, edarkgrey, WHITE);

	apply_rgb = create_button("apply", 0, 1, color_input->outer_box.x + color_input->outer_box.w + 100, color_input->outer_box.y, 0, 0, font, apply, NULL, WHITE, edarkgrey, WHITE);


	return 0;
}

void apply(struct button *self, SDL_Event *e)
{
	if (rgb_mode_ring_type == 0) {
		printf("outer\n");
		int rj = 0;
		for (int i = 0; i < ports[0].fan_count; i++) {
			for (int j = 0; j < 12; j++) {
				ports[0].rgb.outer_color[rj].r = color_buttons[i]->bg_color.r;
				ports[0].rgb.outer_color[rj].g = color_buttons[i]->bg_color.g;
				ports[0].rgb.outer_color[rj].b = color_buttons[i]->bg_color.b;
				rj++;
				printf("rj = %d\n", rj);
				printf("i = %d\n", i);
			}
		}
		set_outer_rgb(&ports[0], &rgb_modes[rgb_mode_i], 0, 0, 0, ports[0].rgb.outer_color);
	}
	if (rgb_mode_ring_type == 1) {
		printf("inner\n");
		int rj = 0;
		for (int i = 0; i < ports[0].fan_count; i++) {
			for (int j = 0; j < 8; j++) {
				ports[0].rgb.inner_color[rj].r = color_buttons[i]->bg_color.r;
				ports[0].rgb.inner_color[rj].g = color_buttons[i]->bg_color.g;
				ports[0].rgb.inner_color[rj].b = color_buttons[i]->bg_color.b;
				rj++;
			}
		}
		set_inner_rgb(&ports[0], &rgb_modes[rgb_mode_i], 0, 0, 0, ports[0].rgb.inner_color);
	}

	if (rgb_mode_ring_type == 2) {
		printf("inner and outer\n");
		int rj = 0, rrj = 0;
		for (int i = 0; i < rgb_modes[rgb_mode_i].colors; i++) {
			for (int j = 0; j < 8; j++) {
				ports[0].rgb.inner_color[rj].r = color_buttons[i]->bg_color.r;
				ports[0].rgb.inner_color[rj].g = color_buttons[i]->bg_color.g;
				ports[0].rgb.inner_color[rj].b = color_buttons[i]->bg_color.b;
				rj++;
			}
			for (int j = 0; j < 12; j++) {
				ports[0].rgb.outer_color[rrj].r = color_buttons[i]->bg_color.r;
				ports[0].rgb.outer_color[rrj].g = color_buttons[i]->bg_color.g;
				ports[0].rgb.outer_color[rrj].b = color_buttons[i]->bg_color.b;
				rrj++;
			}
		}
		printf("stil good\n");
		set_inner_and_outer_rgb(&ports[0], &rgb_modes[rgb_mode_i], 0, 0, 0, ports[0].rgb.outer_color, ports[0].rgb.inner_color);
	}
}

void color_buttons_click(struct button *self, SDL_Event *e)
{
	for (int i = 0; i < 6; i++){
		if (color_buttons[i] == self) {
			selected_color_button = i;
		}
	}
}

void create_color_buttons()
{
	for (int i = 0; i < 6; i++) {
		color_buttons[i] = create_button(NULL, 0, 1, other_picker_pos.x - 35, other_picker_pos.y + 35 * i, 
						30, 30, font, color_buttons_click, NULL, WHITE, RED, WHITE);
		if (i > rgb_modes[rgb_mode_i].colors) color_buttons[i]->show = 0;
	}
}

void rgb_mode_ddm_select(struct drop_down_menu *d, SDL_Event *event)
{
	if (d->selected) {
		rgb_mode_i = d->selected_text_index;
		for (int i = 0; i < 6; i++) {
			if (rgb_modes[rgb_mode_i].colors <= i) color_buttons[i]->show = 0;
			else color_buttons[i]->show = 1;
		}
		for (int i = 0; i < rgb_modes_amount; i++) {
			if (strcmp(rgb_modes[i].name, d->text[d->selected_text_index].str) == 0) rgb_mode_i = i;
		}
		printf("rgb_mode_i = %d, rgb_mode = %s\n", rgb_mode_i, rgb_modes[rgb_mode_i].name);
	}
}

void fan_ring_select(struct drop_down_menu *d, SDL_Event *event)
{
	int strings_added = 0;
	char newstr[50][MAX_TEXT_SIZE];
	rgb_mode_ring_type = d->selected_text_index;
	int mask;
	if (rgb_mode_ring_type == 0) mask = OUTER;
	else if (rgb_mode_ring_type == 1) mask = INNER;
	if (rgb_mode_ring_type == 2) mask = (INNER | OUTER);

	for (int i = 0; i < rgb_modes_amount; i++) {
		if ((rgb_modes[i].outerorinner & mask) == mask) {
			strncpy(newstr[strings_added], rgb_modes[i].name, MAX_TEXT_SIZE);
			strings_added++;
		}
	}
	change_ddm_text_arr(rgb_mode_ddm, strings_added, newstr, font);
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

 /**************************************************************\
 | 								|
 | 	do at some point: come up with a better way for this 	|
 | 								|
 \**************************************************************/
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
		x = (int)x - 1 < 0 ? 0 : x - 1;
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
		black = (float)i / (surface->h-1);
		float newredf   = redf * black;
		float newgreenf = greenf * black;
		float newbluef  = bluef * black;
		Uint8 newred;
		Uint8 newgreen;
		Uint8 newblue;
		for (int j = 0; j < surface->w; j++) {
			float whiter = newredf   + j * ((256-redf)/surface->w) * black;
			float whiteg = newgreenf + j * ((256-greenf)/surface->w) * black;
			float whiteb = newbluef  + j * ((256-bluef)/surface->w) * black;

			newred   = whiter; 
			newgreen = whiteg;
			newblue  = whiteb;
			pixels[i * surface->w + j] = 0xff000000 + (newred << 16) + (newgreen << 8) + newblue;
		}
	}
}

SDL_Surface *create_white_black_picker(Uint8 red, Uint8 green, Uint8 blue)
{
	SDL_Surface *return_surface = SDL_CreateRGBSurface(0, 510, 510, 32, 0, 0, 0, 0);
	Uint32 *pixels = (Uint32 *)return_surface->pixels;
	float black;
	float redf = (float)red;
	float greenf = (float)green;
	float bluef = (float)blue;
	for (int i = 0; i < return_surface->h; i++) {
		black = (float)i / (return_surface->h-1);
		float  newredf   = redf * black;
		float  newgreenf = greenf * black;
		float  newbluef  = bluef * black;
		Uint8 newred;
		Uint8 newgreen;
		Uint8 newblue;
		for (int j = 0; j < return_surface->w; j++) {
			float whiter = newredf   + j * ((256-redf)/return_surface->w) * black;
			float whiteg = newgreenf + j * ((256-greenf)/return_surface->w) * black;
			float whiteb = newbluef  + j * ((256-bluef)/return_surface->w) * black;

			newred   = whiter; 
			newgreen = whiteg;
			newblue  = whiteb;
			pixels[i * return_surface->w + j] = 0xff000000 + (newred << 16) + (newgreen << 8) + newblue;
		}
	}
	return return_surface;
}

void deselect_input(struct input *self, SDL_Event *event)
{
//	color_picker_pos.x = 
}

//reminder uncomment all the comments if you want the box to move x and y
void select_button2(struct button *self, SDL_Event *event)
{
	SDL_MouseMotionEvent mouse_data = event->motion;

	self->outer_box.x = mouse_data.x; 
	//self->outer_box.y = mouse_data.y; 

	if (self->outer_box.x > color_picker_pos.x + color_picker_pos.w-1) {
		self->outer_box.x = color_picker_pos.x + color_picker_pos.w - 1;
	} 
	else if (self->outer_box.x < color_picker_pos.x) {
		self->outer_box.x = color_picker_pos.x;
	} 
	/*if (self->outer_box.y > color_picker_pos.y + color_picker_pos.h-1) {
		self->outer_box.y = color_picker_pos.y + color_picker_pos.h-1;
	}
	else if (self->outer_box.y < color_picker_pos.y) {
		self->outer_box.y = color_picker_pos.y;
	} */

	Uint32 *pixels = (Uint32*)color_picker_surface->pixels;
	//Uint32 pixel = pixels[(self->outer_box.y - color_picker_pos.y) * 
	//				color_picker_surface->w + (self->outer_box.x - color_picker_pos.x)];

	Uint32 pixel = pixels[((self->outer_box.y - color_picker_pos.y)*color_picker_surface->h/color_picker_pos.h) * 
					color_picker_surface->w + ((self->outer_box.x - color_picker_pos.x) * color_picker_surface->w/color_picker_pos.w)];

	color_buttons[selected_color_button]->bg_color.r = self->bg_color.r = pixel >> 16;
	color_buttons[selected_color_button]->bg_color.g = self->bg_color.g = pixel >> 8;
	color_buttons[selected_color_button]->bg_color.b = self->bg_color.b = pixel;
	self->outer_box.x -= self->outer_box.w/2;
	//self->outer_box.y -= self->outer_box.h/2;

	change = 1;
	char new_text[MAX_TEXT_SIZE];
	sprintf(new_text, "%d, %d, %d", self->bg_color.r, self->bg_color.g, self->bg_color.b);
	change_input_box_text(color_input, new_text);
}

void select_button(struct button *self, SDL_Event *event)
{
	SDL_MouseMotionEvent mouse_data = event->motion;

	self->outer_box.x = mouse_data.x; 
	self->outer_box.y = mouse_data.y; 

	if (self->outer_box.x > other_picker_pos.x + other_picker_pos.w) {
		self->outer_box.x = other_picker_pos.x + other_picker_pos.w;
	} 
	else if (self->outer_box.x < other_picker_pos.x) {
		self->outer_box.x = other_picker_pos.x;
	} 

	if (self->outer_box.y > other_picker_pos.y + other_picker_pos.h) {
		self->outer_box.y = other_picker_pos.y + other_picker_pos.h;
	}
	else if (self->outer_box.y < other_picker_pos.y) {
		self->outer_box.y = other_picker_pos.y;
	} 
	int index_y = ((self->outer_box.y - other_picker_pos.y) * other_picker_surface->h / other_picker_pos.h)-1;
	index_y = index_y < 0 ? 0 : index_y;
	int index_x = ((self->outer_box.x - other_picker_pos.x) * other_picker_surface->w / other_picker_pos.w)-1;
	index_x = index_x < 0 ? 0 : index_x;

	Uint32 *pixels = (Uint32*)other_picker_surface->pixels;
	Uint32 pixel = pixels[(index_y) * other_picker_surface->w + (index_x)];

	color_buttons[selected_color_button]->bg_color.r = self->bg_color.r = pixel >> 16;
	color_buttons[selected_color_button]->bg_color.g = self->bg_color.g = pixel >> 8;
	color_buttons[selected_color_button]->bg_color.b = self->bg_color.b = pixel;
	self->outer_box.x -= self->outer_box.w/2;
	self->outer_box.y -= self->outer_box.h/2;

	char new_text[MAX_TEXT_SIZE];
	sprintf(new_text, "%d, %d, %d", self->bg_color.r, self->bg_color.g, self->bg_color.b);
	change_input_box_text(color_input, new_text);
}

























