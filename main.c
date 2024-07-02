 /**************************************************************************************************************\
 |					 	   TODO 							|
 |--------------------------------------------------------------------------------------------------------------|
 |														|
 |		1.  create two input boxes for setting fan curve points.(just implement function callback 	|
 |				on text input or what ever you want to call it ) 				|
 |		2.  finish fan speed control page apply to port apply to all.					|
 | 		3.  finish settings page. 									|
 | 														|
 |--------------------------------------------------------------------------------------------------------------|
 |														|
 |				    not very important but do at some point					|
 |														|
 | 		1.  when a rgb_mode with only INNER_AND_OUTER flag set is applied you try to change 		|
 |		 	inner or outer mode it glithes is basicly fixed but the fix is not perfect so 		|
 |			refactor that since i dont feel like it rn and it works so yk 				|
 |														|
 \**************************************************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>

#include "ui.h"
#include "controller.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int init(void);

/* global varibles */
SDL_Color grey = { 209, 209, 209, SDL_ALPHA_OPAQUE };
SDL_Color darkgrey = { 84, 84, 84, SDL_ALPHA_OPAQUE };
SDL_Color edarkgrey = { 23, 23, 23, 255 };

int change;
int selected_port;
float cpu_temp_num;
char speeds_str[MAX_TEXT_SIZE];
int speeds_pro[4] = { 0 };
int speeds_rpm[4] = { 0 };
int selected_color_button;
int other_index;

/* start rgb page def */
int rgb_mode_i;
int rgb_speed;
int rgb_brightnes;
int direction = 0;
int rgb_merge = 0;
int rgb_mode_mask;
int rgb_mode_ring_type;
struct page *rgb_page;

struct text *rgb_speed_text;
struct text *rgb_brightnes_text;

struct button *fan_page_button_r;
struct button *rgb_page_button_r;
struct button *setting_page_button_r;
struct button *color_buttons[6];
struct button *select_port_rgb_buttons[4];
struct button *direction_buttons[2];
struct button *apply_rgb;
struct button *apply_to_all_rgb;
struct button *toggle_merge;
struct button *color_selector;
struct button *black_white_selector;

struct image *saturation_img;
struct image *color_img;

struct drop_down_menu *rgb_mode_ddm;
struct drop_down_menu *fan_ring_ddm;

struct slider *rgb_brightnes_slider;
struct slider *rgb_speed_slider;

/* rgb functions */
int init_rgb_page(void);
void port_select_fan_func(struct button *self, SDL_Event *event);
inline void create_rgb_color_picker_surface(void);
inline SDL_Surface *create_white_black_picker(Uint8 red, Uint8 green, Uint8 blue);
void rgb_speed_slider_move(struct slider *self, SDL_Event *event);
void rgb_brightnes_slider_move(struct slider *self, SDL_Event *event);
void toggle_merge_button(struct button *self, SDL_Event *event);
void slider_on_release(struct button *self, SDL_Event *e);
void direction_select(struct button *self, SDL_Event *e);
void change_white_black_picker(SDL_Surface *surface, Uint8 red, Uint8 green, Uint8 blue);
void rgb_color_picker_button(struct button *self, SDL_Event *event);
void rgb_saturation_picker_button(struct button *self, SDL_Event *event);
void rgb_mode_ddm_select(struct drop_down_menu *d, SDL_Event *event);
void fan_ring_select(struct drop_down_menu *d, SDL_Event *event);
void color_buttons_click(struct button *self, SDL_Event *e);
void create_color_buttons(void);
void change_colors_to_color_buttons(const struct rgb_mode *new_mode, int led_amount, struct color *colors_to_change);
void change_color_buttons_to_colors(const struct rgb_mode *new_mode, int led_amount, struct color *colors_to_change);
void apply(struct button *self, SDL_Event *e);
void port_select_rgb_func(struct button *self, SDL_Event *event);

/* end rgb page def */

/* start fan page def */
struct page *fan_speed_page;

struct text *cpu_temp;
struct text *graph_fan_speed_text;
struct text *graph_cpu_temp_text;
struct text *speeds;
struct text *port_speed_pro[4];
struct text *port_speed_rpm[4];
struct text *port_text;
struct text *rpm_text;
struct text *pro_text;
struct text *port_nummber[4];

struct line *port_box_lines[6];

struct button *port_bg;
struct button *fan_page_button_f;
struct button *rgb_page_button_f;
struct button *setting_page_button_f;
struct button *select_port_fan_buttons[4];
struct button *apply_fan_speed;

struct graph *fan_curve_graphs[4];
struct point line;

/* fan functions */
int init_fan_page(void);
void change_to_rgb_page(struct button *self, SDL_Event *e);
void moving_graph(struct graph *self, SDL_Event *e);
int update_speed_str(void);
void change_to_fan_page(struct button *self, SDL_Event *e);
void update_temp(void);
float get_fan_speed_from_graph(struct graph *g, float temp);
void apply_fans_func(struct button *self, SDL_Event *e);

/* end fan page def */

/* start settings page def */
struct page *settings_page;

struct button *fan_page_button_s;
struct button *rgb_page_button_s;
struct button *setting_page_button_s;
struct button *fan_count_buttons[4][6];

/* settings functions */
int init_settings_page(void);
void fan_count_button_click(struct button *self, SDL_Event *e);
void change_to_settings_page(struct button *self, SDL_Event *e);

/* end settings page def */

int main(void)
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
		if (delta2 > 1000/2.0) {
			b2 = a;
			if (showen_page == fan_speed_page) {
				update_temp();
				update_speed_str();		
			}
		}
		SDL_Delay(1);
	}
	for (int i = 0; i < 4; i++) {
		save_port(&ports[i]);
	}
	ui_shutdown();

	return 0;
}

int init(void)
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

	init_fan_page();
	init_rgb_page();
	init_settings_page();

	show_page(rgb_page);

	return 0;
}

int init_fan_page(void)
{
	fan_speed_page = create_page();
	for (int i = 0; i < 4; i++) {
		load_port(&ports[i]);
		fan_curve_graphs[i] = create_graph(20, WINDOW_H-220, 100, 100, 4, 2, 10, 10, 10, 10, 0, moving_graph, WHITE, BLACK, BLUE, grey, BLUE, fan_speed_page);
		fan_curve_graphs[i]->points = realloc(fan_curve_graphs[i]->points, sizeof(struct point) * ports[i].points_total);
		mempcpy(fan_curve_graphs[i]->points, ports[i].curve, sizeof(struct point) * ports[i].points_total);
		fan_curve_graphs[i]->point_amount = ports[i].points_used;
		fan_curve_graphs[i]->total_points = ports[i].points_total;
		fan_curve_graphs[i]->show = 0;
		fan_curve_graphs[i]->rerender = 0;
	}
	fan_curve_graphs[0]->show = 1;
	fan_curve_graphs[0]->rerender = 1;


	port_bg = create_button(NULL, 0, 1, 425, 100, 300, 191, 0, font, NULL, NULL, WHITE, BLACK, WHITE, fan_speed_page);

	port_box_lines[0] = create_line(port_bg->outer_box.x + port_bg->outer_box.w/3, port_bg->outer_box.y, 
			port_bg->outer_box.x + port_bg->outer_box.w/3, port_bg->outer_box.y + port_bg->outer_box.h - 31, WHITE, fan_speed_page);
	port_box_lines[0]->show = 1;
	port_box_lines[1] = create_line(port_bg->outer_box.x + port_bg->outer_box.w/1.5, port_bg->outer_box.y, 
			port_bg->outer_box.x + port_bg->outer_box.w/1.5, port_bg->outer_box.y + port_bg->outer_box.h - 31, WHITE, fan_speed_page);
	port_box_lines[1]->show = 1;

	port_text = create_text("PORTS", 426, 106, 0, 0, 20, 0, WHITE, BLACK, font, fan_speed_page);
	rpm_text = create_text("RPM", port_box_lines[0]->start.x + 5, 106, 0, 0, 20, 0, WHITE, BLACK, font, fan_speed_page);
	pro_text = create_text("%", port_box_lines[1]->start.x + 5, 106, 0, 0, 20, 0, WHITE, BLACK, font, fan_speed_page);
	port_box_lines[2] = create_line(port_bg->outer_box.x, port_text->dst.y + port_text->dst.h + 1, 
			port_bg->outer_box.x + port_bg->outer_box.w - 2, port_text->dst.y + port_text->dst.h + 1, WHITE, fan_speed_page);
	port_box_lines[2]->show = 1;


	for (int i = 0; i < 4; i++) {
		char tmp_str[4];
		sprintf(tmp_str, "#%d", i);
		port_nummber[i] = create_text(tmp_str, port_text->dst.x + 2, port_text->dst.y + port_text->dst.h + 5, 0, 0, 23, 0, WHITE, port_bg->bg_color, font, fan_speed_page);
		port_nummber[i]->dst.y += (port_nummber[i]->dst.h + 5) * i;
		port_speed_pro[i] = create_text("0", pro_text->dst.x, port_nummber[i]->dst.y, 0, 0, 23, 0, WHITE, port_bg->bg_color, font, fan_speed_page);
		port_speed_rpm[i] = create_text("0", rpm_text->dst.x, port_nummber[i]->dst.y, 0, 0, 23, 0, WHITE, port_bg->bg_color, font, fan_speed_page);
		port_box_lines[i + 2] = create_line(port_bg->outer_box.x, port_nummber[i]->dst.y + port_nummber[i]->dst.h + 2, 
				port_bg->outer_box.x + port_bg->outer_box.w - 2, port_nummber[i]->dst.y + port_nummber[i]->dst.h + 2, WHITE, fan_speed_page);
	}
	if (update_speed_str() != 0) {
		printf("update_speed_str failed");
		return -1;
	}
	//speeds = create_text(speeds_str, 425, 250, 0, 0, 0, 0, WHITE, edarkgrey, font, fan_speed_page);

	cpu_temp = create_text("current cpu temp: 0°", port_bg->outer_box.x + 5, port_box_lines[5]->start.y + 3, 0, 0, 20, 0, WHITE, BLACK, font, fan_speed_page);
	
	graph_cpu_temp_text = create_text("cputemp: 0°", 20, 220, 0, 0, 20, 0, WHITE, edarkgrey, font, fan_speed_page);
	graph_fan_speed_text = create_text("fanspeed: 0%", 20, 250, 0, 0, 20, 0, WHITE, edarkgrey, font, fan_speed_page);

	int prev_w = 0, prev_h = 0;
	for (int i = 0; i < 4; i++) {
		char portstr[7];
		sprintf(portstr, "Port %d", i+1); 
		select_port_fan_buttons[i] = create_button(portstr, 0, 1, fan_curve_graphs[0]->scaled_pos.x + fan_curve_graphs[0]->scaled_pos.w + prev_w + 10, 
					fan_curve_graphs[0]->scaled_pos.y + fan_curve_graphs[0]->scaled_pos.h - 73 + prev_h, 0, 0, 0, font, port_select_fan_func, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);
		if ((i + 1) % 2 == 0) {
			prev_h = select_port_fan_buttons[i]->outer_box.h + 5;
			prev_w = 0;
		} else {
			prev_w = select_port_fan_buttons[i]->outer_box.w + 5;
		}
	}
	apply_fan_speed = create_button("Apply", 0, 1, select_port_fan_buttons[3]->outer_box.x + select_port_fan_buttons[3]->outer_box.w, select_port_fan_buttons[3]->outer_box.y, 0, 0, 0, font, apply_fans_func, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);

	create_input("0", "cpu", 0, 3, 200, 10, 0, 0, NULL, font, WHITE, BLACK, WHITE, fan_speed_page);

	setting_page_button_f = create_button("settings", 0, 1, 10, 10, 0, 0, 20, font, change_to_settings_page, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);
	rgb_page_button_f = create_button("rgb", 0, 1, 10, setting_page_button_f->outer_box.y + setting_page_button_f->outer_box.h + 5,
			0, 0, 20, font, change_to_rgb_page, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);
	rgb_page_button_f->outer_box.w = setting_page_button_f->outer_box.w;
	fan_page_button_f = create_button("fan", 0, 1, 10, rgb_page_button_f->outer_box.y + rgb_page_button_f->outer_box.h + 5, 
			0, 0, 20, font, change_to_fan_page, NULL, WHITE, grey, BLACK, fan_speed_page);
	fan_page_button_f->outer_box.w = setting_page_button_f->outer_box.w;
	return 0;
}

int init_rgb_page(void)
{
	rgb_page = create_page();
	rgb_brightnes = ports[0].rgb.inner_brightnes;

	rgb_speed = ports[0].rgb.inner_speed;

	direction = ports[0].rgb.inner_direction;

	color_img = create_image(10, WINDOW_H-125, 300, 20, 1530, 1, 32, rgb_page);
	create_rgb_color_picker_surface();
	color_img->texture = create_texture_from_surface(color_img->surface);

	color_selector = create_button(NULL, 1, 1, color_img->pos.x, color_img->pos.y, 20, 20, 0, font, rgb_saturation_picker_button, rgb_saturation_picker_button, WHITE, BLACK, WHITE, rgb_page);

	saturation_img = create_image(color_img->pos.x, color_img->pos.y - 215, 300, 200, 510, 510, 32, rgb_page);
	saturation_img->surface = create_white_black_picker(255, 0, 0);
	saturation_img->texture = create_texture_from_surface(saturation_img->surface);

	black_white_selector = create_button(NULL, 1, 1, saturation_img->pos.x, saturation_img->pos.y, 25, 25, 0, font, rgb_color_picker_button, rgb_color_picker_button, WHITE, BLACK, WHITE, rgb_page);
	
	create_color_buttons();
	char tmp_str[rgb_modes_amount][MAX_TEXT_SIZE];
	for (int i = 0; i < rgb_modes_amount; i++) {
		strncpy(tmp_str[i], rgb_modes[i].name, MAX_TEXT_SIZE);
	}

	setting_page_button_r = create_button("settings", 0, 1, 10, 10, 0, 0, 20, font, change_to_settings_page, NULL, WHITE, edarkgrey, WHITE, rgb_page);
	rgb_page_button_r = create_button("rgb", 0, 1, 10, setting_page_button_r->outer_box.y + setting_page_button_r->outer_box.h + 5,
			0, 0, 20, font, change_to_rgb_page, NULL, WHITE, grey, BLACK, rgb_page);
	rgb_page_button_r->outer_box.w = setting_page_button_r->outer_box.w;
	fan_page_button_r = create_button("fan", 0, 1, 10, rgb_page_button_r->outer_box.y + rgb_page_button_r->outer_box.h + 5, 
			0, 0, 20, font, change_to_fan_page, NULL, WHITE, edarkgrey, WHITE, rgb_page);
	fan_page_button_r->outer_box.w = setting_page_button_r->outer_box.w;

	rgb_mode_ddm = create_drop_down_menu(rgb_modes_amount, tmp_str, 45, fan_page_button_r->outer_box.y + fan_page_button_r->outer_box.h + 5, 150, 0, 150, 300, rgb_mode_ddm_select, font, WHITE, edarkgrey, WHITE, rgb_page);
	char str[][MAX_TEXT_SIZE] = { "O", "I", "OI" };
	fan_ring_ddm = create_drop_down_menu(3, str, 10, fan_page_button_r->outer_box.y + fan_page_button_r->outer_box.h + 5, 0, 0, 0, 70, fan_ring_select, font, WHITE, edarkgrey, WHITE, rgb_page);

	int prev_w = 0, prev_h = 0;
	for (int i = 0; i < 4; i++) {
		char portstr[7];
		sprintf(portstr, "Port %d", i+1); 
		select_port_rgb_buttons[i] = create_button(portstr, 0, 1, 330 + prev_w, saturation_img->pos.y + saturation_img->pos.h - 73 + prev_h, 0, 0, 0, font, port_select_rgb_func, NULL, WHITE, edarkgrey, WHITE, rgb_page);
		if ((i + 1) % 2 == 0) {
			prev_h = select_port_rgb_buttons[i]->outer_box.h + 5;
			prev_w = 0;
		} else {
			prev_w = select_port_rgb_buttons[i]->outer_box.w + 5;
		}
	}
	
	direction_buttons[0] = create_button("<<<", 0, 1, 325, saturation_img->pos.y + saturation_img->pos.h, 0, 0, 40, font, direction_select, NULL, edarkgrey, edarkgrey, WHITE, rgb_page);
	direction_buttons[1] = create_button(">>>", 0, 1, 325 + direction_buttons[0]->outer_box.w, direction_buttons[0]->outer_box.y, 0, 0, 40, font, direction_select, NULL, edarkgrey, edarkgrey, WHITE, rgb_page);

	apply_rgb = create_button("Apply", 0, 1, color_img->pos.x, color_buttons[0]->outer_box.y + color_buttons[0]->outer_box.h + 10, 0, 0, 0, font, apply, NULL, WHITE, edarkgrey, WHITE, rgb_page);
	apply_to_all_rgb = create_button("Apply all", 0, 1, apply_rgb->outer_box.x + apply_rgb->outer_box.w + 5, apply_rgb->outer_box.y, 0, 0, 0, font, apply, NULL, WHITE, edarkgrey, WHITE, rgb_page);

	rgb_speed_slider = create_slider(1, 330, 200, 200, 10, 20, slider_on_release, rgb_speed_slider_move, WHITE, WHITE, darkgrey, rgb_page);
	rgb_brightnes_slider = create_slider(1, 330, 240, 200, 10, 20, slider_on_release, rgb_brightnes_slider_move, WHITE, WHITE, darkgrey, rgb_page);

	rgb_speed_text = create_text("0%", rgb_speed_slider->pos.x + 210, rgb_speed_slider->button->outer_box.y - 2, 0, 0, 0, 0, WHITE, edarkgrey, font, rgb_page);
	rgb_brightnes_text = create_text("0%", rgb_brightnes_slider->pos.x + 210, rgb_brightnes_slider->button->outer_box.y - 2, 0, 0, 0 , 0, WHITE, edarkgrey, font, rgb_page);

	float rounded = 0.0;
	if (rgb_brightnes == 0x08) 
		rounded = rgb_brightnes_slider->p = 0.00;
	else if (rgb_brightnes == 0x03) 
		rounded = rgb_brightnes_slider->p = 0.25;
	else if (rgb_brightnes == 0x02) 
		rounded = rgb_brightnes_slider->p = 0.50;
	else if (rgb_brightnes == 0x01) 
		rounded = rgb_brightnes_slider->p = 0.75;
	else 
		rounded = rgb_brightnes_slider->p = 1.00;
			
	char bs_str[6];
	sprintf(bs_str, "%3d%%", (int)(rounded * 100));
	change_text_and_render_texture(rgb_brightnes_text, bs_str, WHITE, edarkgrey, font);
	rgb_brightnes_slider->button->outer_box.x = rgb_brightnes_slider->pos.x + (rgb_brightnes_slider->pos.w * rounded) - rgb_brightnes_slider->button->outer_box.w/2;

	rounded = 0.0;
	if (rgb_speed == 0x02) 
		rounded = rgb_speed_slider->p = 0.00;
	else if (rgb_speed == 0x01) 
		rounded = rgb_speed_slider->p = 0.25;
	else if (rgb_speed == 0x00) 
		rounded = rgb_speed_slider->p = 0.50;
	else if (rgb_speed == 0xff) 
		rounded = rgb_speed_slider->p = 0.75;
	else 
		rounded = rgb_speed_slider->p = 1.00;

	sprintf(bs_str, "%3d%%", (int)(rounded * 100));
	change_text_and_render_texture(rgb_speed_text, bs_str, WHITE, edarkgrey, font);
	rgb_speed_slider->button->outer_box.x = rgb_speed_slider->pos.x + (rgb_speed_slider->pos.w * rounded) - rgb_speed_slider->button->outer_box.w/2;

	toggle_merge = create_button("Merge", 0, 1, apply_to_all_rgb->outer_box.x + apply_to_all_rgb->outer_box.w + 5, apply_to_all_rgb->outer_box.y, 0, 0, 20, font, toggle_merge_button, NULL, WHITE, edarkgrey, WHITE, rgb_page);
	
	return 0;
}

int init_settings_page(void)
{
	settings_page = create_page();
	int total_y = 150, total_x = 10;
	for (int p = 0; p < 4; p++) {
		for (int f = 0; f < 6; f++) {
			fan_count_buttons[p][f] = create_button(NULL, 0, 1, total_x, total_y, 30, 30, 20, font, fan_count_button_click, NULL, WHITE, BLACK, WHITE, settings_page);
			if (f < ports[p].fan_count) fan_count_buttons[p][f]->bg_color = WHITE;
			total_x += fan_count_buttons[p][f]->outer_box.w + 5;
		}
		total_x = 10;
		total_y += fan_count_buttons[p][0]->outer_box.h + 5;
	}
	setting_page_button_s = create_button("settings", 0, 1, 10, 10, 0, 0, 20, font, change_to_settings_page, NULL, WHITE, grey, BLACK, settings_page);
	rgb_page_button_s = create_button("rgb", 0, 1, 10, setting_page_button_s->outer_box.y + setting_page_button_s->outer_box.h + 5,
			0, 0, 20, font, change_to_rgb_page, NULL, WHITE, edarkgrey, WHITE, settings_page);
	rgb_page_button_s->outer_box.w = setting_page_button_s->outer_box.w;
	fan_page_button_s = create_button("fan", 0, 1, 10, rgb_page_button_s->outer_box.y + rgb_page_button_s->outer_box.h + 5, 
			0, 0, 20, font, change_to_fan_page, NULL, WHITE, edarkgrey, WHITE, settings_page);
	fan_page_button_s->outer_box.w = setting_page_button_s->outer_box.w;
	return 0;
}

void fan_count_button_click(struct button *self, SDL_Event *e)
{
	int selected_p = 0, selected_f = 0;
	for (int p = 0; p < 4; p++) {
		for (int f = 0; f < 6; f++) {
			if (self == fan_count_buttons[p][f]) {
				ports[p].fan_count = f+1;
				set_fan_count(&ports[p], f+1);
				printf("port %d fan_count = %d\n", p, f+1);
				selected_p = p;
				selected_f = f;
			}  
		}
	}
	for (int i = 0; i < 6; i++) {
		if (i <= selected_f) {
			fan_count_buttons[selected_p][i]->bg_color = WHITE;
		} else if (i > selected_f) {
			fan_count_buttons[selected_p][i]->bg_color = BLACK;
		}
	}
}

void apply_fans_func(struct button *self, SDL_Event *e)
{
	if (fan_curve_graphs[selected_port]->point_amount > ports[selected_port].points_total) {
		ports[selected_port].curve = realloc(ports[selected_port].curve, sizeof(struct port) * fan_curve_graphs[selected_port]->point_amount);
		ports[selected_port].points_total = fan_curve_graphs[selected_port]->point_amount;
	}
	set_fan_curve(&ports[selected_port]);
}

void change_to_settings_page(struct button *self, SDL_Event *e)
{
	show_page(settings_page);
}
void change_to_rgb_page(struct button *self, SDL_Event *e)
{
	show_page(rgb_page);
}

void change_to_fan_page(struct button *self, SDL_Event *e)
{
	show_page(fan_speed_page);
}

void update_temp(void) {
	FILE *fcpu = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	int icputemp;
	fscanf(fcpu, "%d", &icputemp);
	cpu_temp_num = icputemp/1000.0;
	char str[25];
	sprintf(str, "current cpu temp: %.2f°", cpu_temp_num); 
	change_text_and_render_texture(cpu_temp, str, cpu_temp->fg_color, cpu_temp->bg_color, font);
	fan_curve_graphs[selected_port]->x.x = cpu_temp_num;
	fclose(fcpu);
}

void moving_graph(struct graph *self, SDL_Event *e)
{
	if (self->selected_point != NULL) {
		char tmp_str[25];
		sprintf(tmp_str, "fanspeed: %3d%%", 100-self->selected_point->y );
		change_text_and_render_texture(graph_fan_speed_text, tmp_str, WHITE, edarkgrey, font);
		sprintf(tmp_str, "cputemp: %3d°", self->selected_point->x); 
		change_text_and_render_texture(graph_cpu_temp_text, tmp_str, WHITE, edarkgrey, font);
	}
}

void toggle_merge_button(struct button *self, SDL_Event *event)
{
	rgb_merge = (rgb_merge + 1) % 2;
	if (rgb_merge) {
		self->bg_color = WHITE;
		render_text_texture(self->text, BLACK, WHITE, font);
	} else {
		self->bg_color = edarkgrey;
		render_text_texture(self->text, WHITE, edarkgrey, font);
	}

}

void port_select_fan_func(struct button *self, SDL_Event *event) 
{
	for (int i = 0; i < 4; i++) {
		if (self == select_port_fan_buttons[i]) {
			fan_curve_graphs[selected_port]->show = 0;
			fan_curve_graphs[selected_port]->rerender = 0;
			selected_port = i;
			fan_curve_graphs[selected_port]->show = 1;
			fan_curve_graphs[selected_port]->rerender = 1;
			select_port_fan_buttons[i]->bg_color = WHITE;
			render_text_texture(select_port_fan_buttons[i]->text, BLACK, WHITE, font);
		} else {
			select_port_fan_buttons[i]->bg_color = edarkgrey;
			render_text_texture(select_port_fan_buttons[i]->text, WHITE, edarkgrey, font);
		}
	}
}
void port_select_rgb_func(struct button *self, SDL_Event *event) 
{
	for (int i = 0; i < 4; i++) {
		if (self == select_port_rgb_buttons[i]) {
			selected_port = i;
			select_port_rgb_buttons[i]->bg_color = WHITE;
			render_text_texture(select_port_rgb_buttons[i]->text, BLACK, WHITE, font);
		} else {
			select_port_rgb_buttons[i]->bg_color = edarkgrey;
			render_text_texture(select_port_rgb_buttons[i]->text, WHITE, edarkgrey, font);
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
				if (i < ports[0].fan_count) {
					color_buttons[i]->bg_color.r = colors_to_change[rj].r;
					color_buttons[i]->bg_color.g = colors_to_change[rj].g;
					color_buttons[i]->bg_color.b = colors_to_change[rj].b;
				} else {
					color_buttons[i]->bg_color.r = 255;
					color_buttons[i]->bg_color.g = 0;
					color_buttons[i]->bg_color.b = 0;

				}
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
	int set_alls = self == apply_to_all_rgb ? 1 : 0;
	int led_amount = 0, port = set_alls ? 0 : selected_port;

	if (rgb_mode_ring_type == 0) {
		led_amount = 12;
		for (int i = 0; i <= 3 * set_alls; i++) {
			change_colors_to_color_buttons(&rgb_modes[rgb_mode_i], led_amount, ports[port].rgb.outer_color);
			port++;
		}
		set_outer_rgb(&ports[selected_port], &rgb_modes[rgb_mode_i], rgb_speed, direction, rgb_brightnes, set_alls, ports[selected_port].rgb.outer_color, 1);
	} else if (rgb_mode_ring_type == 1) {
		led_amount = 8;
		for (int i = 0; i <= 3 * set_alls; i++) {
			change_colors_to_color_buttons(&rgb_modes[rgb_mode_i], led_amount, ports[port].rgb.inner_color);
			port++;
		}
		set_inner_rgb(&ports[selected_port], &rgb_modes[rgb_mode_i], rgb_speed, direction, rgb_brightnes, set_alls, ports[selected_port].rgb.inner_color, 1);
	} else if (rgb_mode_ring_type == 2) {
		if (rgb_merge == 0) {
			for (int i = 0; i <= 3 * set_alls; i++) {
				led_amount = 12;
				change_colors_to_color_buttons(&rgb_modes[rgb_mode_i], led_amount, ports[port].rgb.outer_color);
				led_amount = 8;
				change_colors_to_color_buttons(&rgb_modes[rgb_mode_i], led_amount, ports[port].rgb.inner_color);
				port++;
			}
			set_inner_and_outer_rgb(&ports[selected_port], &rgb_modes[rgb_mode_i], rgb_speed, direction, rgb_brightnes, set_alls, ports[selected_port].rgb.outer_color, ports[selected_port].rgb.inner_color);
		} else {
			led_amount = 8;
			change_colors_to_color_buttons(&rgb_modes[rgb_mode_i], led_amount, ports[port].rgb.inner_color);
			printf("merge now\n");
			set_merge(&ports[selected_port], &rgb_modes[rgb_mode_i], rgb_speed, direction, rgb_brightnes, ports[selected_port].rgb.inner_color);
		}
	}

	printf("port = %d, mode = %s, 0x%02x, set all = %d, speed = 0x%02x, brightnes = 0x%02x\n", selected_port, rgb_modes[rgb_mode_i].name, rgb_modes[rgb_mode_i].mode, set_alls, rgb_speed, rgb_brightnes);
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

void create_color_buttons(void)
{
	for (int i = 0; i < 6; i++) {
		color_buttons[i] = create_button(NULL, 0, 1, saturation_img->pos.x + 51 * i, color_img->pos.y + color_img->pos.h + 10, 
						45, 40, 0, font, color_buttons_click, NULL, WHITE, RED, WHITE, rgb_page);
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


int update_speed_str(void)
{
	for (int i = 0; i < 4; i++) {
		speeds_pro[i] = get_fan_speed_pro(ports[i].proc_path);
		speeds_rpm[i] = get_fan_speed_rpm(ports[i].proc_path);
		sprintf(port_speed_pro[i]->str, "%d%%", speeds_pro[i]);
		sprintf(port_speed_rpm[i]->str, "%d", speeds_rpm[i]);
		change_text_and_render_texture(port_speed_pro[i], port_speed_pro[i]->str, port_speed_pro[i]->fg_color, port_speed_pro[i]->bg_color, font);
		change_text_and_render_texture(port_speed_rpm[i], port_speed_rpm[i]->str, port_speed_rpm[i]->fg_color, port_speed_rpm[i]->bg_color, font);
	}
	return 0;
}

 /**************************************************************\
 | 								|
 | 	do at some point: come up with a better way for this 	|
 | 								|
 \**************************************************************/
void create_rgb_color_picker_surface(void)
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

void rgb_saturation_picker_button(struct button *self, SDL_Event *event)
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

	self->bg_color.r = pixel >> 16;
	self->bg_color.g = pixel >> 8;
	self->bg_color.b = pixel;
	self->outer_box.x -= self->outer_box.w/2;

	change = 1;
}

void rgb_color_picker_button(struct button *self, SDL_Event *event)
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
