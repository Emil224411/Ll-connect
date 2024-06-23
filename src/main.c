 /**************************************************************************************************************\
 |														|
 |		currently main.c is mostly just testing stuff but some of it is going to stay			|
 |														|
 |--------------------------------------------------------------------------------------------------------------|
 |														|
 |						TODO list							|
 |														|
 | 		1.  in kernel module implement fan curves and FIX TIMER for checking cpu temp.			|
 | 		2.  in kernel module implement saving and loading.						|
 | 		3.  create settings page for setting fan count and what ever else there is a need for.		|
 |		4.  have a look at the input box code and create two input boxes for setting a points x 	|
 |				and y.										|
 |		5.  finish fan speed control page apply to port apply to all.					|
 | 														|
 |--------------------------------------------------------------------------------------------------------------|
 |														|
 |				    not very important but do at some point					|
 |														|
 | 		1.  when a rgb_mode with only INNER_AND_OUTER flag set is applied you try to change 		|
 |		 	inner or outer mode it glithes is basicly fixed but the fix is not perfect so 		|
 |			refactor that since i dont feel like it rn and it works so yk 				|
 | 		2. test which rgb modes require NOT_MOVING flag(and find a new name for the flag 		|
 |			since Breathing needs the flag)(i dont think any more modes need it) 			|
 |		3. mabey write functions for setting colors and setting the mode in the driver 			|
 | 			look at usbNotes.txt:41.								|
 |														|
 \**************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "../include/ui.h"
#include "../include/controller.h"


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

SDL_Color grey = { 209, 209, 209, SDL_ALPHA_OPAQUE };
SDL_Color darkgrey = { 84, 84, 84, SDL_ALPHA_OPAQUE };
SDL_Color edarkgrey = { 23, 23, 23, 255 };

void moving_graph(struct graph *self, SDL_Event *e);

inline void create_rgb_color_picker_surface();
inline SDL_Surface *create_white_black_picker(Uint8 red, Uint8 green, Uint8 blue);
void rgb_speed_slider_move(struct slider *self, SDL_Event *event);
void rgb_brightnes_slider_move(struct slider *self, SDL_Event *event);
void toggle_merge_button(struct button *self, SDL_Event *event);
void slider_on_release(struct button *self, SDL_Event *e);
void direction_select(struct button *self, SDL_Event *e);
void change_white_black_picker(SDL_Surface *surface, Uint8 red, Uint8 green, Uint8 blue);
void select_button(struct button *self, SDL_Event *event);
void select_button2(struct button *self, SDL_Event *event);
void deselect_input(struct input *self, SDL_Event *event);
void rgb_mode_ddm_select(struct drop_down_menu *d, SDL_Event *event);
void fan_ring_select(struct drop_down_menu *d, SDL_Event *event);
void color_buttons_click(struct button *self, SDL_Event *e);
void create_color_buttons();
void change_colors_to_color_buttons(const struct rgb_mode *new_mode, int led_amount, struct color *colors_to_change);
void change_color_buttons_to_colors(const struct rgb_mode *new_mode, int led_amount, struct color *colors_to_change);
void apply(struct button *self, SDL_Event *e);
void port_select_rgb_func(struct button *self, SDL_Event *event);
void port_select_fan_func(struct button *self, SDL_Event *event);
int update_speed_str();
int init();
static void set_all(int mode, int speed, int direction, int brightnes, int flag);
void change_to_rgb_page(struct button *self, SDL_Event *e);
void change_to_fan_page(struct button *self, SDL_Event *e);
void update_temp();
float get_fan_speed_from_graph(struct graph *g, float temp);
void apply_fans_func(struct button *self, SDL_Event *e);

float cpu_temp_num;

struct text *cpu_temp;

struct page *test_page;
struct page *test_page2;

struct image *saturation_img;
struct image *color_img;

struct button *rgb_page_button_f;
struct button *rgb_page_button_r;
struct button *fan_page_button_f;
struct button *fan_page_button_r;

struct text *graph_fan_speed_text;
struct text *graph_cpu_temp_text;
struct graph *test_graph;
char speeds_str[MAX_TEXT_SIZE];
struct button *apply_fan_speed;
struct text *speeds;
int speeds_pro[4] = { 0 };
int speeds_rpm[4] = { 0 };
struct button *color_selector;
struct button *black_white_selector;
int change;
//struct input *color_input_r;
//struct input *color_input_g;
//struct input *color_input_b;
struct text *color_text;

struct point line;

struct slider *rgb_speed_slider;
struct text *rgb_speed_text;
int rgb_speed;
struct slider *rgb_brightnes_slider;
struct text *rgb_brightnes_text;
int rgb_brightnes;
struct button *direction_buttons[2];
int direction = 0;
struct button *color_buttons[6];
int selected_color_button;
struct button *apply_rgb;
struct button *apply_to_all_rgb;
struct button *toggle_merge;
int rgb_merge = 0;
struct drop_down_menu *rgb_mode_ddm;
struct drop_down_menu *fan_ring_ddm;
int rgb_mode_i;
int rgb_mode_ring_type;
int rgb_mode_mask;
struct button *select_port_rgb_buttons[4];
struct button *select_port_fan_buttons[4];
int selected_port;
int other_index;

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
				change_white_black_picker(saturation_img->surface, color_selector->bg_color.r, color_selector->bg_color.g, color_selector->bg_color.b);
				SDL_DestroyTexture(saturation_img->texture);
				saturation_img->texture = create_texture_from_surface(saturation_img->surface);
				change = 0;
			}
		
			render_showen_page();
			
			show_screen();
		}
		if (delta2 > 1000/1.0) {
			b2 = a;
			update_temp();
			update_speed_str();		
			change_text_and_render_texture(speeds, speeds_str, speeds->fg_color, speeds->bg_color, font);
		}
		SDL_Delay(1);
	}
	for (int i = 0; i < 4; i++) save_port(&ports[i]);
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

	test_page = create_page();
	test_page2 = create_page();
	for (int i = 0; i < 4; i++) {
		ports[i].fan_curve = create_graph(20, WINDOW_H-220, 100, 100, 4, 2, 10, 10, 10, 10, 0, moving_graph, WHITE, BLACK, BLUE, grey, BLUE, test_page2);
		ports[i].fan_curve->show = 0;
		ports[i].fan_curve->rerender = 0;
		load_port(&ports[i]);
	}
	ports[0].fan_curve->show = 1;
	ports[0].fan_curve->rerender = 1;
	rgb_brightnes = ports[0].rgb.inner_brightnes;

	rgb_speed = ports[0].rgb.inner_speed;

	direction = ports[0].rgb.inner_direction;
	speeds = create_text(speeds_str, 10, 100, 0, 0, 0, 0, WHITE, edarkgrey, font, test_page2);

	color_img = create_image(10, WINDOW_H-130, 300, 20, 1530, 1, 32, test_page);
	create_rgb_color_picker_surface();
	color_img->texture = create_texture_from_surface(color_img->surface);

	color_selector = create_button(NULL, 1, 1, color_img->pos.x, color_img->pos.y, 20, 20, 0, font, select_button2, select_button2, WHITE, BLACK, WHITE, test_page);

	saturation_img = create_image(color_img->pos.x, color_img->pos.y - 215, 300, 200, 510, 510, 32, test_page);
	saturation_img->surface = create_white_black_picker(255, 0, 0);
	saturation_img->texture = create_texture_from_surface(saturation_img->surface);

	black_white_selector = create_button(NULL, 1, 1, saturation_img->pos.x, saturation_img->pos.y, 25, 25, 0, font, select_button, select_button, WHITE, BLACK, WHITE, test_page);
	
	create_color_buttons();
	char tmp_str[rgb_modes_amount][MAX_TEXT_SIZE];
	for (int i = 0; i < rgb_modes_amount; i++) {
		strncpy(tmp_str[i], rgb_modes[i].name, MAX_TEXT_SIZE);
	}
	
	rgb_mode_ddm = create_drop_down_menu(rgb_modes_amount, tmp_str, 45, 120, 150, 0, 150, 300, rgb_mode_ddm_select, font, WHITE, edarkgrey, WHITE, test_page);
	char str[][MAX_TEXT_SIZE] = { "O", "I", "OI" };
	fan_ring_ddm = create_drop_down_menu(3, str, 10, 120, 0, 0, 0, 70, fan_ring_select, font, WHITE, edarkgrey, WHITE, test_page);

	int prev_w = 0, prev_h = 0;
	for (int i = 0; i < 4; i++) {
		char portstr[7];
		sprintf(portstr, "Port %d", i+1); 
		select_port_rgb_buttons[i] = create_button(portstr, 0, 1, 330 + prev_w, saturation_img->pos.y + saturation_img->pos.h - 73 + prev_h, 0, 0, 0, font, port_select_rgb_func, NULL, WHITE, edarkgrey, WHITE, test_page);
		if ((i + 1) % 2 == 0) {
			prev_h = select_port_rgb_buttons[i]->outer_box.h + 5;
			prev_w = 0;
		} else {
			prev_w = select_port_rgb_buttons[i]->outer_box.w + 5;
		}
	}
	
	direction_buttons[0] = create_button("<<<", 0, 1, 325, saturation_img->pos.y + saturation_img->pos.h, 0, 0, 40, font, direction_select, NULL, edarkgrey, edarkgrey, WHITE, test_page);
	direction_buttons[1] = create_button(">>>", 0, 1, 325 + direction_buttons[0]->outer_box.w, direction_buttons[0]->outer_box.y, 0, 0, 40, font, direction_select, NULL, edarkgrey, edarkgrey, WHITE, test_page);

	apply_rgb = create_button("Apply", 0, 1, color_img->pos.x, color_buttons[0]->outer_box.y + color_buttons[0]->outer_box.h + 10, 0, 0, 0, font, apply, NULL, WHITE, edarkgrey, WHITE, test_page);
	apply_to_all_rgb = create_button("Apply all", 0, 1, apply_rgb->outer_box.x + apply_rgb->outer_box.w + 5, apply_rgb->outer_box.y, 0, 0, 0, font, apply, NULL, WHITE, edarkgrey, WHITE, test_page);

	rgb_speed_slider = create_slider(1, 330, 200, 200, 10, 20, slider_on_release, rgb_speed_slider_move, WHITE, WHITE, darkgrey, test_page);
	rgb_brightnes_slider = create_slider(1, 330, 240, 200, 10, 20, slider_on_release, rgb_brightnes_slider_move, WHITE, WHITE, darkgrey, test_page);

	rgb_speed_text = create_text("0%", rgb_speed_slider->pos.x + 210, rgb_speed_slider->button->outer_box.y - 2, 0, 0, 0, 0, WHITE, edarkgrey, font, test_page);
	rgb_brightnes_text = create_text("0%", rgb_brightnes_slider->pos.x + 210, rgb_brightnes_slider->button->outer_box.y - 2, 0, 0, 0 , 0, WHITE, edarkgrey, font, test_page);

	float rounded = 0.0;
	switch (rgb_brightnes) {
		case 0x08:
			rounded = rgb_brightnes_slider->p = 0.00;
			break;
		case 0x03:
			rounded = rgb_brightnes_slider->p = 0.25;
			break;
		case 0x02:
			rounded = rgb_brightnes_slider->p = 0.50;
			break;
		case 0x01:
			rounded = rgb_brightnes_slider->p = 0.75;
			break;
		case 0x00:
			rounded = rgb_brightnes_slider->p = 1.00;
			break;
			
			
	}
	char brightnes_str[6];
	sprintf(brightnes_str, "%3d%%", (int)(rounded * 100));
	change_text_and_render_texture(rgb_brightnes_text, brightnes_str, WHITE, edarkgrey, font);
	rgb_brightnes_slider->button->outer_box.x = rgb_brightnes_slider->pos.x + (rgb_brightnes_slider->pos.w * rounded) - rgb_brightnes_slider->button->outer_box.w/2;
	rounded = 0.0;
	switch (rgb_speed) {
		case 0x02:
			rounded = rgb_speed_slider->p = 0.00;
			break;
		case 0x01:
			rounded = rgb_speed_slider->p = 0.25;
			break;
		case 0x00:
			rounded = rgb_speed_slider->p = 0.50;
			break;
		case 0xff:
			rounded = rgb_speed_slider->p = 0.75;
			break;
		case 0xfe:
			rounded = rgb_speed_slider->p = 1.00;
			break;
			
			
	}
	sprintf(brightnes_str, "%3d%%", (int)(rounded * 100));
	change_text_and_render_texture(rgb_speed_text, brightnes_str, WHITE, edarkgrey, font);
	rgb_speed_slider->button->outer_box.x = rgb_speed_slider->pos.x + (rgb_speed_slider->pos.w * rounded) - rgb_speed_slider->button->outer_box.w/2;
	toggle_merge = create_button("Merge", 0, 1, apply_to_all_rgb->outer_box.x + apply_to_all_rgb->outer_box.w + 5, apply_to_all_rgb->outer_box.y, 0, 0, 20, font, toggle_merge_button, NULL, WHITE, edarkgrey, WHITE, test_page);
	
	graph_cpu_temp_text = create_text("cputemp: %", 20, 220, 0, 0, 20, 0, WHITE, edarkgrey, font, test_page2);
	graph_fan_speed_text = create_text("fanspeed: %", 20, 250, 0, 0, 20, 0, WHITE, edarkgrey, font, test_page2);
	prev_w = 0, prev_h = 0;
	for (int i = 0; i < 4; i++) {
		char portstr[7];
		sprintf(portstr, "Port %d", i+1); 
		select_port_fan_buttons[i] = create_button(portstr, 0, 1, ports[0].fan_curve->scaled_pos.x + ports[0].fan_curve->scaled_pos.w + prev_w + 10, 
					ports[0].fan_curve->scaled_pos.y + ports[0].fan_curve->scaled_pos.h - 73 + prev_h, 0, 0, 0, font, port_select_fan_func, NULL, WHITE, edarkgrey, WHITE, test_page2);
		if ((i + 1) % 2 == 0) {
			prev_h = select_port_fan_buttons[i]->outer_box.h + 5;
			prev_w = 0;
		} else {
			prev_w = select_port_fan_buttons[i]->outer_box.w + 5;
		}
	}
	apply_fan_speed = create_button("Apply", 0, 1, select_port_fan_buttons[3]->outer_box.x + select_port_fan_buttons[3]->outer_box.w, select_port_fan_buttons[3]->outer_box.y, 0, 0, 0, font, apply_fans_func, NULL, WHITE, edarkgrey, WHITE, test_page2);

	rgb_page_button_r = create_button("rgb", 0, 1, 10, 20, 0, 0, 20, font, change_to_rgb_page, NULL, WHITE, edarkgrey, WHITE, test_page);
	fan_page_button_r = create_button("fan", 0, 1, 10, rgb_page_button_r->outer_box.y + rgb_page_button_r->outer_box.h + 5, 
					0, 0, 20, font, change_to_fan_page, NULL, WHITE, edarkgrey, WHITE, test_page);
	rgb_page_button_f = create_button("rgb", 0, 1, 10, 20, 0, 0, 20, font, change_to_rgb_page, NULL, WHITE, edarkgrey, WHITE, test_page2);
	fan_page_button_f = create_button("fan", 0, 1, 10, rgb_page_button_f->outer_box.y + rgb_page_button_f->outer_box.h + 5, 
					0, 0, 20, font, change_to_fan_page, NULL, WHITE, edarkgrey, WHITE, test_page2);
	cpu_temp = create_text("0", 100, 150, 0, 0, 20, 0, WHITE, edarkgrey, font, test_page2);
	show_page(test_page);

	return 0;
}

void apply_fans_func(struct button *self, SDL_Event *e)
{
	set_fan_speed(&ports[selected_port], get_fan_speed_from_graph(ports[selected_port].fan_curve, cpu_temp_num));
}

float get_fan_speed_from_graph(struct graph *g, float temp) 
{
	for (int i = 0; i < ports[selected_port].fan_curve->point_amount; i++) {
		float xone = (float)ports[selected_port].fan_curve->points[i].x, xtwo = (float)ports[selected_port].fan_curve->points[i + 1].x;
		float yone = 100.0 - ports[selected_port].fan_curve->points[i].y, ytwo = 100.0 - ports[selected_port].fan_curve->points[i + 1].y;
		if (xone < temp && xtwo > temp) {
			float a = (ytwo - yone)/(xtwo - xone);
			float b = yone - a * xone;
			return a * temp + b;
		}
	}
	return 0.0;
}

void change_to_rgb_page(struct button *self, SDL_Event *e)
{
	show_page(test_page);
}

void change_to_fan_page(struct button *self, SDL_Event *e)
{
	show_page(test_page2);
}

void update_temp() {
	FILE *fcpu = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	int icputemp;
	fscanf(fcpu, "%d", &icputemp);
	cpu_temp_num = icputemp/1000.0;
	char str[10];
	sprintf(str, "%.2f", cpu_temp_num); 
	change_text_and_render_texture(cpu_temp, str, cpu_temp->fg_color, cpu_temp->bg_color, font);
	ports[selected_port].fan_curve->x.x = cpu_temp_num;
	fclose(fcpu);
}

void moving_graph(struct graph *self, SDL_Event *e)
{
	if (self->selected_point != NULL) {
		char tmp_str[16];
		sprintf(tmp_str, "fanspeed: %d%%", 100-self->selected_point->y );
		change_text_and_render_texture(graph_fan_speed_text, tmp_str, WHITE, edarkgrey, font);
		sprintf(tmp_str, "cputemp: %d°", self->selected_point->x); 
		change_text_and_render_texture(graph_cpu_temp_text, tmp_str, WHITE, edarkgrey, font);
	}
}

void toggle_merge_button(struct button *self, SDL_Event *event)
{
	rgb_merge = (rgb_merge + 1) % 2;
	self->bg_color = rgb_merge ? WHITE : edarkgrey;
}

void port_select_fan_func(struct button *self, SDL_Event *event) 
{
	for (int i = 0; i < 4; i++) {
		if (self == select_port_fan_buttons[i]) {
			ports[selected_port].fan_curve->show = 0;
			ports[selected_port].fan_curve->rerender = 0;
			selected_port = i;
			ports[i].fan_curve->show = 1;
			ports[i].fan_curve->rerender = 1;
		}
	}
}
void port_select_rgb_func(struct button *self, SDL_Event *event) 
{
	for (int i = 0; i < 4; i++) {
		if (self == select_port_rgb_buttons[i]) {
			selected_port = i;
		}
	}
}
void direction_select(struct button *self, SDL_Event *e)
{
	if (self == direction_buttons[0]) {
		direction = 1;
		render_text_texture(direction_buttons[1]->text, WHITE, edarkgrey, font);
	} else if (self == direction_buttons[1]) {
		direction = 0;
		render_text_texture(direction_buttons[0]->text, WHITE, edarkgrey, font);
	}
	render_text_texture(self->text, BLUE, edarkgrey, font);
}

void slider_on_release(struct button *self, SDL_Event *e) 
{
	self->bg_color = WHITE;
}

/*
 * brightnes:
 *	0%   = 08
 *	25%  = 03
 *	50%  = 02
 *	75%  = 01
 *	100% = 00
 */
void rgb_brightnes_slider_move(struct slider *self, SDL_Event *event)
{
	static float last_p;
	float rounded;
	if      (self->p == 0.00) { rounded = 0.00; rgb_brightnes = 0x08; }
	else if (self->p == 0.25) { rounded = 0.25; rgb_brightnes = 0x03; }
	else if (self->p == 0.50) { rounded = 0.50; rgb_brightnes = 0x02; }
	else if (self->p == 0.75) { rounded = 0.75; rgb_brightnes = 0x01; }
	else if (self->p == 1.00) { rounded = 1.00; rgb_brightnes = 0x00; }
	else rounded = last_p;
	if (rounded != last_p) {
		char tmp_str[6];
		sprintf(tmp_str, "%3d%%", (int)(rounded * 100));
		change_text_and_render_texture(rgb_brightnes_text, tmp_str, WHITE, edarkgrey, font);
	}
	self->button->outer_box.x = self->pos.x + (self->pos.w * rounded) - self->button->outer_box.w/2;
	last_p = rounded;
	self->button->bg_color = darkgrey;
}
/*
 * speed:
 *	0%   = 02 
 *	25%  = 01
 *	50%  = 00 
 *	75%  = ff   
 *	100% = fe
 */
void rgb_speed_slider_move(struct slider *self, SDL_Event *event)
{
	static float last_p;
	float rounded;
	if      (self->p == 0.00) { rounded = 0.00; rgb_speed = 0x02; }
	else if (self->p == 0.25) { rounded = 0.25; rgb_speed = 0x01; }
	else if (self->p == 0.50) { rounded = 0.50; rgb_speed = 0x00; }
	else if (self->p == 0.75) { rounded = 0.75; rgb_speed = 0xff; }
	else if (self->p == 1.00) { rounded = 1.00; rgb_speed = 0xfe; }
	else rounded = last_p;
	if (rounded != last_p) {
		char tmp_str[6];
		sprintf(tmp_str, "%3d%%", (int)(rounded * 100));
		change_text_and_render_texture(rgb_speed_text, tmp_str, WHITE, edarkgrey, font);
	}

	self->button->outer_box.x = self->pos.x + (self->pos.w * rounded) - self->button->outer_box.w/2;
	last_p = rounded;
	self->button->bg_color = darkgrey;
}

void change_colors_to_color_buttons(const struct rgb_mode *new_mode, int led_amount, struct color *colors_to_change)
{
	int rj = 0;
	if (new_mode->flags & NOT_MOVING) {
		for (int i = 0; i < new_mode->colors; i++) {
			for (int j = 0; j < led_amount; j++) {
				colors_to_change[rj].r = color_buttons[i]->bg_color.r;
				colors_to_change[rj].g = color_buttons[i]->bg_color.g;
				colors_to_change[rj].b = color_buttons[i]->bg_color.b;
				rj++;
			}
		}
	} else if (new_mode->colors > 0) {
		for (int j = 0; j < ports[selected_port].fan_count; j++) {
			for (int i = 0; i < new_mode->colors; i++) {
				colors_to_change[j * ports[selected_port].fan_count + i].r = color_buttons[i]->bg_color.r;
				colors_to_change[j * ports[selected_port].fan_count + i].g = color_buttons[i]->bg_color.g;
				colors_to_change[j * ports[selected_port].fan_count + i].b = color_buttons[i]->bg_color.b;
			}
		}
	} else memset(colors_to_change, 0, sizeof(struct color) * led_amount * 6);

}

void change_color_buttons_to_colors(const struct rgb_mode *new_mode, int led_amount, struct color *colors_to_change)
{
	int rj = 0;
	if (new_mode->flags & NOT_MOVING) {
		for (int i = 0; i < new_mode->colors; i++) {
			for (int j = 0; j < led_amount; j++) {
				color_buttons[i]->bg_color.r = colors_to_change[rj].r;
				color_buttons[i]->bg_color.g = colors_to_change[rj].g;
				color_buttons[i]->bg_color.b = colors_to_change[rj].b;
				rj++;
			}
		}
	} else if (new_mode->colors > 0) {
		for (int j = 0; j < ports[selected_port].fan_count; j++) {
			for (int i = 0; i < new_mode->colors; i++) {
				color_buttons[i]->bg_color.r = colors_to_change[j * ports[selected_port].fan_count + i].r;
				color_buttons[i]->bg_color.g = colors_to_change[j * ports[selected_port].fan_count + i].g;
				color_buttons[i]->bg_color.b = colors_to_change[j * ports[selected_port].fan_count + i].b;
			}
		}
	}

}

void apply(struct button *self, SDL_Event *e)
{
	int set_all = self == apply_to_all_rgb ? 1 : 0;
	int led_amount = 0, port = set_all ? 0 : selected_port;
	struct color *color_to_change;

	if (rgb_mode_ring_type == 0) {
		led_amount = 12;
		for (int i = 0; i <= 3 * set_all; i++) {
			change_colors_to_color_buttons(&rgb_modes[rgb_mode_i], led_amount, ports[port].rgb.outer_color);
			port++;
		}
		set_outer_rgb(&ports[selected_port], &rgb_modes[rgb_mode_i], rgb_speed, direction, rgb_brightnes, set_all, ports[selected_port].rgb.outer_color, 1);
	} else if (rgb_mode_ring_type == 1) {
		led_amount = 8;
		for (int i = 0; i <= 3 * set_all; i++) {
			change_colors_to_color_buttons(&rgb_modes[rgb_mode_i], led_amount, ports[port].rgb.inner_color);
			port++;
		}
		set_inner_rgb(&ports[selected_port], &rgb_modes[rgb_mode_i], rgb_speed, direction, rgb_brightnes, set_all, ports[selected_port].rgb.inner_color, 1);
	} else if (rgb_mode_ring_type == 2) {
		int flag = rgb_modes[rgb_mode_i].flags;
		if (rgb_merge == 0) {
			for (int i = 0; i <= 3 * set_all; i++) {
				led_amount = 12;
				change_colors_to_color_buttons(&rgb_modes[rgb_mode_i], led_amount, ports[port].rgb.outer_color);
				led_amount = 8;
				change_colors_to_color_buttons(&rgb_modes[rgb_mode_i], led_amount, ports[port].rgb.inner_color);
				port++;
			}
			set_inner_and_outer_rgb(&ports[selected_port], &rgb_modes[rgb_mode_i], rgb_speed, direction, rgb_brightnes, set_all, ports[selected_port].rgb.outer_color, ports[selected_port].rgb.inner_color);
		} else {
			led_amount = 8;
			change_colors_to_color_buttons(&rgb_modes[rgb_mode_i], led_amount, ports[port].rgb.inner_color);
			printf("merge now\n");
			set_merge(&ports[selected_port], &rgb_modes[rgb_mode_i], rgb_speed, direction, rgb_brightnes, ports[selected_port].rgb.inner_color);
		}
	}

	printf("port = %d, mode = %s, 0x%02x, set all = %d, speed = 0x%02x, brightnes = 0x%02x\n", selected_port, rgb_modes[rgb_mode_i].name, rgb_modes[rgb_mode_i].mode, set_all, rgb_speed, rgb_brightnes);
}

void color_buttons_click(struct button *self, SDL_Event *e)
{
	for (int i = 0; i < 6; i++){
		if (color_buttons[i] == self) {
			selected_color_button = i;
			color_buttons[i]->bg_color.r = black_white_selector->bg_color.r;
			color_buttons[i]->bg_color.g = black_white_selector->bg_color.g;
			color_buttons[i]->bg_color.b = black_white_selector->bg_color.b;
		}
	}
}

void create_color_buttons()
{
	for (int i = 0; i < 6; i++) {
		color_buttons[i] = create_button(NULL, 0, 1, saturation_img->pos.x + 51 * i, color_img->pos.y + color_img->pos.h + 10, 
						45, 40, 0, font, color_buttons_click, NULL, WHITE, RED, WHITE, test_page);
		if (i > rgb_modes[rgb_mode_i].colors) color_buttons[i]->show = 0;
	}
	change_color_buttons_to_colors(ports[0].rgb.outer_mode, 12, ports[0].rgb.outer_color);
}

void rgb_mode_ddm_select(struct drop_down_menu *d, SDL_Event *event)
{
	if (d->selected) {
		for (int i = 0; i < rgb_modes_amount; i++) {
			if (strcmp(rgb_modes[i].name, d->text[d->selected_text_index]->str) == 0 
					&& (rgb_modes[i].flags & rgb_mode_mask) == rgb_mode_mask) {
				rgb_mode_i = i;
				rgb_brightnes_text->show = rgb_brightnes_slider->show = (rgb_modes[i].flags & BRIGHTNESS) ? 1 : 0;
				if (rgb_brightnes_text->show == 0) rgb_brightnes = 0;
				else if (rgb_mode_ring_type == 0 || rgb_mode_ring_type == 2) rgb_brightnes = ports[selected_port].rgb.inner_brightnes;
				else if (rgb_mode_ring_type == 1) rgb_brightnes = ports[selected_port].rgb.outer_brightnes;
				rgb_speed_text->show = rgb_speed_slider->show = (rgb_modes[i].flags & SPEED) ? 1 : 0;
				if (rgb_speed_text->show == 0) rgb_speed = 0;
				else if (rgb_mode_ring_type == 0 || rgb_mode_ring_type == 2) rgb_speed = ports[selected_port].rgb.inner_speed;
				else if (rgb_mode_ring_type == 1) rgb_speed = ports[selected_port].rgb.outer_speed;
				direction = direction_buttons[0]->show = direction_buttons[1]->show = (rgb_modes[i].flags & DIRECTION) ? 1 : 0;
				toggle_merge->show = (rgb_modes[i].flags & MERGE) && rgb_mode_ring_type == 2 ? 1 : 0;
				if (toggle_merge->show == 0) rgb_merge = 0;
			}
		}
		for (int i = 0; i < 6; i++) {
			if (rgb_modes[rgb_mode_i].colors <= i) color_buttons[i]->show = 0;
			else color_buttons[i]->show = 1;
		}
	}
}

void fan_ring_select(struct drop_down_menu *d, SDL_Event *event)
{
	int strings_added = 0;
	char newstr[50][MAX_TEXT_SIZE];
	rgb_mode_ring_type = d->selected_text_index;
	if      (rgb_mode_ring_type == 0) rgb_mode_mask = OUTER;
	else if (rgb_mode_ring_type == 1) rgb_mode_mask = INNER;
	else if (rgb_mode_ring_type == 2) rgb_mode_mask = INNER_AND_OUTER;

	for (int i = 0; i < rgb_modes_amount; i++) {
		if ((rgb_modes[i].flags & rgb_mode_mask) == rgb_mode_mask) {
			strncpy(newstr[strings_added], rgb_modes[i].name, MAX_TEXT_SIZE);
			strings_added++;
		}
	}
	change_ddm_text_arr(rgb_mode_ddm, strings_added, newstr, font);
}


int update_speed_str()
{
	for (int i = 0; i < 4; i++) {
		speeds_pro[i] = get_fan_speed_pro(ports[i].proc_path);
		speeds_rpm[i] = get_fan_speed_rpm(ports[i].proc_path);
	}

	sprintf(speeds_str, "%d, %d, %d, %d\n%d, %d, %d, %d", 
			speeds_pro[0], speeds_pro[1], speeds_pro[2], speeds_pro[3], 
			speeds_rpm[0], speeds_rpm[1], speeds_rpm[2], speeds_rpm[3]);

	return 0;
}

 /**************************************************************\
 | 								|
 | 	do at some point: come up with a better way for this 	|
 | 								|
 \**************************************************************/
void create_rgb_color_picker_surface()
{
	Uint32 *rgb = (Uint32 *)color_img->surface->pixels;
	int colorthing = 1530/6;
	Uint8 red = 0xff, green = 0, blue = 0;
	/*
	 * TODO 
	Uint8 *c = &redgreenblue[1];
	Uint8  ci = 0;
	int8_t add = -1;
	for (int i = 0; i < 1530; i++) {
		if (i % colorthing == 0) {
			add = -add;
			c = &redgreenblue[ci];
			ci = (ci + 1) % 3;
			printf("ci = %d\n", ci);
			printf("add = %d\n", add);
		}
		rgb[i] = 0xff000000 + (redgreenblue[0] << 16) + (redgreenblue[1] << 8) + redgreenblue[2];
		*c = *c + add;
		printf("%08x\n", rgb[i]);
	}*/
	int tmp_i = 0;
	for(int i = 0; i < colorthing; i++) {
		green = i;
		rgb[/*y * return_surface->w + */i +(int)(colorthing * 0)] = 0xff000000 + (red << 16) + (green << 8) + blue;
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
		tmp_i++;
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

	Uint8 red_index = 0, green_index = 0, blue_index = 0, redtwo_index = 0;
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
	Uint32 pixel = pixels[other_index];
	black_white_selector->bg_color.r = pixel >> 16;
	black_white_selector->bg_color.g = pixel >> 8;
	black_white_selector->bg_color.b = pixel;

	//char new_text[4];
	//sprintf(new_text, "%d", black_white_selector->bg_color.r);
	//change_input_box_text(color_input_r, new_text);
	//sprintf(new_text, "%d", black_white_selector->bg_color.g);
	//change_input_box_text(color_input_g, new_text);
	//sprintf(new_text, "%d", black_white_selector->bg_color.b);
	//change_input_box_text(color_input_b, new_text);
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

void select_button2(struct button *self, SDL_Event *event)
{
	SDL_MouseMotionEvent mouse_data = event->motion;

	self->outer_box.x = mouse_data.x; 

	if (self->outer_box.x > color_img->pos.x + color_img->pos.w-1) {
		self->outer_box.x = color_img->pos.x + color_img->pos.w - 1;
	} 
	else if (self->outer_box.x < color_img->pos.x) {
		self->outer_box.x = color_img->pos.x;
	} 
	Uint32 *pixels = (Uint32*)color_img->surface->pixels;

	Uint32 pixel = pixels[((self->outer_box.y - color_img->pos.y)*color_img->surface->h/color_img->pos.h) * 
					color_img->surface->w + ((self->outer_box.x - color_img->pos.x) * color_img->surface->w/color_img->pos.w)];
	int mouseindex = ((self->outer_box.y - color_img->pos.y)*color_img->surface->h/color_img->pos.h) * 
                           	color_img->surface->w + ((self->outer_box.x - color_img->pos.x) * color_img->surface->w/color_img->pos.w);

	self->bg_color.r = pixel >> 16;
	self->bg_color.g = pixel >> 8;
	self->bg_color.b = pixel;
	self->outer_box.x -= self->outer_box.w/2;

	change = 1;
}

void select_button(struct button *self, SDL_Event *event)
{
	SDL_MouseMotionEvent mouse_data = event->motion;

	self->outer_box.x = mouse_data.x; 
	self->outer_box.y = mouse_data.y; 

	if (self->outer_box.x > saturation_img->pos.x + saturation_img->pos.w) {
		self->outer_box.x = saturation_img->pos.x + saturation_img->pos.w;
	} else if (self->outer_box.x < saturation_img->pos.x) {
		self->outer_box.x = saturation_img->pos.x;
	} 
	if (self->outer_box.y > saturation_img->pos.y + saturation_img->pos.h) {
		self->outer_box.y = saturation_img->pos.y + saturation_img->pos.h;
	} else if (self->outer_box.y < saturation_img->pos.y) {
		self->outer_box.y = saturation_img->pos.y;
	} 
	int index_y = ((self->outer_box.y - saturation_img->pos.y) * saturation_img->surface->h / saturation_img->pos.h)-1;
	index_y = index_y < 0 ? 0 : index_y;
	int index_x = ((self->outer_box.x - saturation_img->pos.x) * saturation_img->surface->w / saturation_img->pos.w)-1;
	index_x = index_x < 0 ? 0 : index_x;
	other_index = index_y * saturation_img->surface->w + index_x;

	Uint32 *pixels = (Uint32*)saturation_img->surface->pixels;
	Uint32 pixel = pixels[other_index];

	self->bg_color.r = pixel >> 16;
	self->bg_color.g = pixel >> 8;
	self->bg_color.b = pixel;
	self->outer_box.x -= self->outer_box.w/2;
	self->outer_box.y -= self->outer_box.h/2;

	//char new_text[4];
	//sprintf(new_text, "%d", black_white_selector->bg_color.r);
	//change_input_box_text(color_input_r, new_text);
	//sprintf(new_text, "%d", black_white_selector->bg_color.g);
	//change_input_box_text(color_input_g, new_text);
	//sprintf(new_text, "%d", black_white_selector->bg_color.b);
	//change_input_box_text(color_input_b, new_text);
	
}

























