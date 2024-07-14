 /**************************************************************************************************************\
 |					 	   TODO 							|
 |--------------------------------------------------------------------------------------------------------------|
 | 														|
 | 		1.  getting "corrupted double-linked list" on shutdown happens when saving curves 		|
 |			and segfault on startup sometimes in reallocate_curve gdb says: 			|
 |			 __GI___libc_realloc (oldmem=0x696c67756265645f, bytes=61043536) at malloc.c:3440 	|
 |			in reallocate_curve(additional_points=5) at controller.c:539 				|
 | 			in load_curve("port 1\n", "/.config/Ll-connect-config/fan_curve_0") at controller.c:589 |
 |			in init_fan_curve_conf () at controller.c:122 						|
 |			in init_controller () at controller.c:174 						|
 | 														|
 |		2.  finish fan speed control page apply to port apply to all.					|
 | 		3.  finish settings page. 									|
 | 														|
 |--------------------------------------------------------------------------------------------------------------|
 |														|
 |				    not very important but do at some point					|
 |														|
 | 		1.  when you turn on you pc the dev_probe function gets called twice which is why i got 	|
 |			the error proc entry "/proc/Lian_li_hub" all ready exists i have created a tmp fix 	|
 |			but i should figure out a better solution. 						|
 | 		2.  when a rgb_mode with only INNER_AND_OUTER flag set is applied you try to change 		|
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
struct text *remove_curve_text;

/* functions */
int init(void);
void add_fan_curve(struct point *p);
void blink_input(void);
void on_remove_prompt_show(void);

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
struct text *port_speed_pro[4];
struct text *port_speed_rpm[4];
struct text *port_text;
struct text *rpm_text;
struct text *pro_text;
struct text *port_nummber[4];

struct line *port_box_lines[6];

struct button *graph_fan_speed_text;
struct button *graph_cpu_temp_text;
struct button *port_bg;
struct button *fan_page_button_f;
struct button *rgb_page_button_f;
struct button *setting_page_button_f;
struct button *select_port_fan_buttons[4];
struct button *apply_fan_curve;
struct button *apply_all_fan_curve;
struct button *save_curve_button;
struct button *remove_curve_b;
struct button *add_curve_b;

struct drop_down_menu *fan_curve_ddm;

struct graph *fan_curve_graph;
struct point line;

struct input *cpu_temp_input;
struct input *fan_speed_input;

struct prompt *add_fan_curve_prompt;
struct prompt *remove_curve_prompt;
struct callback *blink_cb;
struct callback *speed_temp_update_cb;

/* fan functions */
int init_fan_page(void);
int destroy_fan_page(void);
void change_to_rgb_page(struct button *self, SDL_Event *e);
void moving_graph(struct graph *self, SDL_Event *e);
int update_speed_str(void);
void change_to_fan_page(struct button *self, SDL_Event *e);
void update_temp(void);
float get_fan_speed_from_graph(struct graph *g, float temp);
void apply_fans_func(struct button *self, SDL_Event *e);
void save_curve_bf(struct button *self, SDL_Event *e);
int filter_cpu_input(struct input *self, char new_text[32]);
void select_curve_ddm(struct drop_down_menu *self, SDL_Event *e);
void remove_curve_bf(struct button *self, SDL_Event *e);
void add_curve_bf(struct button *self, SDL_Event *e);
void done_prompt(struct button *self, SDL_Event *e);
void canncel_addc_prompt(struct button *self, SDL_Event *e);
void yes_remove_prompt(struct button *self, SDL_Event *e);
void canncel_remove_prompt(struct button *self, SDL_Event *e);
void update_stuff(void);

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
	double delta = 0;
	
	running = 1;
	while (running) {
		a = SDL_GetTicks();
		delta = a - b;

		check_events_and_callbacks(&event);
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
		SDL_Delay(1);
	}
	ui_shutdown();
	shutdown_controller();

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
	init_controller();

	init_fan_page();
	init_rgb_page();
	init_settings_page();

	port_select_fan_func(select_port_fan_buttons[0], NULL);
	show_page(rgb_page);

	return 0;
}

int init_fan_page(void)
{
	fan_speed_page = create_page();
	for (int i = 0; i < 4; i++) load_port(&ports[i]);

	port_bg = create_button(NULL, 0, 1, 425, WINDOW_H - 211, 300, 191, 0, font, NULL, NULL, WHITE, BLACK, WHITE, fan_speed_page);

	port_box_lines[0] = create_line(port_bg->pos.x + port_bg->pos.w/3, port_bg->pos.y, 
			port_bg->pos.x + port_bg->pos.w/3, port_bg->pos.y + port_bg->pos.h - 31, WHITE, fan_speed_page);
	port_box_lines[0]->show = 1;
	port_box_lines[1] = create_line(port_bg->pos.x + port_bg->pos.w/1.5, port_bg->pos.y, 
			port_bg->pos.x + port_bg->pos.w/1.5, port_bg->pos.y + port_bg->pos.h - 31, WHITE, fan_speed_page);
	port_box_lines[1]->show = 1;

	port_text = create_text("PORTS", 426, WINDOW_H - 205, 0, 0, 20, 0, WHITE, font, fan_speed_page);
	rpm_text = create_text("RPM", port_box_lines[0]->start.x + 5, WINDOW_H - 205, 0, 0, 20, 0, WHITE, font, fan_speed_page);
	pro_text = create_text("%", port_box_lines[1]->start.x + 5, WINDOW_H - 205, 0, 0, 20, 0, WHITE, font, fan_speed_page);
	port_box_lines[2] = create_line(port_bg->pos.x, port_text->dst.y + port_text->dst.h + 1, 
			port_bg->pos.x + port_bg->pos.w - 2, port_text->dst.y + port_text->dst.h + 1, WHITE, fan_speed_page);
	port_box_lines[2]->show = 1;


	for (int i = 0; i < 4; i++) {
		char tmp_str[4];
		sprintf(tmp_str, "#%d", i + 1);
		port_nummber[i] = create_text(tmp_str, port_text->dst.x + 2, port_text->dst.y + port_text->dst.h + 5, 0, 0, 23, 0, WHITE, font, fan_speed_page);
		port_nummber[i]->dst.y += (port_nummber[i]->dst.h + 5) * i;
		port_speed_pro[i] = create_text("0", pro_text->dst.x, port_nummber[i]->dst.y, 0, 0, 23, 0, WHITE, font, fan_speed_page);
		port_speed_rpm[i] = create_text("0", rpm_text->dst.x, port_nummber[i]->dst.y, 0, 0, 23, 0, WHITE, font, fan_speed_page);
		port_box_lines[i + 2] = create_line(port_bg->pos.x, port_nummber[i]->dst.y + port_nummber[i]->dst.h + 2, 
				port_bg->pos.x + port_bg->pos.w - 2, port_nummber[i]->dst.y + port_nummber[i]->dst.h + 2, WHITE, fan_speed_page);
	}
	if (update_speed_str() != 0) {
		printf("update_speed_str failed");
		return -1;
	}
	//speeds = create_text(speeds_str, 425, 250, 0, 0, 0, 0, WHITE, edarkgrey, font, fan_speed_page);

	cpu_temp = create_text("current cpu temp: 0°", port_bg->pos.x + 5, port_box_lines[5]->start.y + 3, 0, 0, 20, 0, WHITE, font, fan_speed_page);
	
	graph_fan_speed_text = create_button("fan speed :", 0, 1, 20, 200, 0, 0, 20, font, NULL, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);
	graph_cpu_temp_text  = create_button("cpu temp :", 0, 1, 20, 205 + graph_fan_speed_text->pos.h, 0, 0, 20, font, NULL, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);
	graph_cpu_temp_text->pos.w = graph_fan_speed_text->pos.w;

	cpu_temp_input = create_input("0", "cpu", 0, 3, 25 + graph_cpu_temp_text->pos.w, 205 + graph_cpu_temp_text->pos.h, 0, 0, NULL, font, WHITE, edarkgrey, WHITE, fan_speed_page);

	cpu_temp_input->filter = filter_cpu_input;

	fan_speed_input = create_input("0", "fan", 0, 3, 25 + graph_fan_speed_text->pos.w, 200, 0, 0, NULL, font, WHITE, edarkgrey, WHITE, fan_speed_page);

	fan_speed_input->filter = filter_cpu_input;

	int prev_w = 0, prev_h = 0;
	for (int i = 0; i < 4; i++) {
		char portstr[7];
		sprintf(portstr, "Port %d", i+1); 
		select_port_fan_buttons[i] = create_button(portstr, 0, 1, cpu_temp_input->pos.x + cpu_temp_input->pos.w + prev_w + 10, 
					200 + prev_h, 0, 0, 0, font, port_select_fan_func, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);
		if ((i + 1) % 2 == 0) {
			prev_h = select_port_fan_buttons[i]->pos.h + 5;
			prev_w = 0;
		} else {
			prev_w = select_port_fan_buttons[i]->pos.w + 5;
		}
	}
	select_port_fan_buttons[0]->bg_color = WHITE;
	render_text_texture(select_port_fan_buttons[0]->text, BLACK, font);
	apply_fan_curve = create_button("Apply", 0, 1, select_port_fan_buttons[1]->pos.x + select_port_fan_buttons[1]->pos.w + 5, select_port_fan_buttons[1]->pos.y, 0, 0, 0, font, apply_fans_func, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);

	char tmp_ddm_str[fan_curve_arr_len][MAX_TEXT_SIZE];
	for (int i = 0; i < fan_curve_arr_len; i++) {
		strcpy(tmp_ddm_str[i], fan_curve_arr[i].name);
	}
	fan_curve_ddm = create_drop_down_menu(fan_curve_arr_len, tmp_ddm_str, select_port_fan_buttons[3]->pos.x + select_port_fan_buttons[3]->pos.w + 5, 
			select_port_fan_buttons[3]->pos.y, 0, select_port_fan_buttons[3]->pos.h, 0, 100, select_curve_ddm, font, WHITE, edarkgrey, WHITE, fan_speed_page);
	save_curve_button = create_button("Save", 0, 1, apply_fan_curve->pos.x + fan_curve_ddm->default_pos.w + 5, fan_curve_ddm->default_pos.y,
			0, 0, 20, font, save_curve_bf, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);

	fan_curve_graph = create_graph(20, WINDOW_H-220, 100, 100, 4, 2, 10, 10, 10, 10, 0, moving_graph, WHITE, BLACK, BLUE, grey, BLUE, fan_speed_page);
	change_graph_points(fan_curve_graph, fan_curve_arr[ports[0].curve_i].curve, fan_curve_arr[ports[0].curve_i].used_points);
	fan_curve_graph->on_click = moving_graph;
	fan_curve_graph->show = 1;
	fan_curve_graph->rerender = 1;
	select_ddm_item(fan_curve_ddm, ports[0].curve_i);

	remove_curve_b = create_button("remove", 0, 1, 20, 160, 0, 0, 20, font, remove_curve_bf, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);
	add_curve_b = create_button("add", 0, 1, 25 + remove_curve_b->pos.w, 160, 0, 0, 20, font, add_curve_bf, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);
	remove_curve_prompt = create_prompt(WINDOW_W/2-150, WINDOW_H/2-100, 300, 200, edarkgrey, WHITE, font);
	remove_curve_prompt->on_show = on_remove_prompt_show;
	add_fan_curve_prompt = create_prompt(WINDOW_W/2-150, WINDOW_H/2-100, 300, 200, edarkgrey, WHITE, font);
	struct text *tmp = create_text("enter name of new fan curve", 0, 0, 0, 0, 17, 0, WHITE, font, NULL);
	tmp->dst.x = 150 - tmp->dst.w/2;
	tmp->dst.y = 10;
	tmp->show = 1;
	struct input *tmp_in = create_input("", "name", 0, 0, tmp->dst.x, tmp->dst.y + tmp->dst.h + 10, tmp->dst.w, 0, NULL, font, WHITE, BLACK, WHITE, NULL);
	tmp_in->default_text->fg_color.r *= 0.5;
	tmp_in->default_text->fg_color.g *= 0.5;
	tmp_in->default_text->fg_color.b *= 0.5;
	tmp_in->default_text->fg_color.a *= 0.5;
	render_text_texture(tmp_in->default_text, tmp_in->default_text->fg_color, font);
	struct button *tmp_b = create_button("done", 0, 1, 10, 200, 0, 0, 20, font, done_prompt, NULL, WHITE, edarkgrey, WHITE, NULL);
	tmp_b->pos.y -= tmp_b->pos.h + 10;
	tmp_b->text->dst.y -= tmp_b->pos.h + 10;
	add_button_to_prompt(add_fan_curve_prompt, tmp_b);
	tmp_b = create_button("yes", 0, 1, 10, 200, 0, 0, 20, font, yes_remove_prompt, NULL, WHITE, edarkgrey, WHITE, NULL);
	tmp_b->pos.y -= tmp_b->pos.h + 10;
	tmp_b->text->dst.y -= tmp_b->pos.h + 10;
	add_button_to_prompt(remove_curve_prompt, tmp_b);

	tmp_b = create_button("canncel", 0, 1, 300, 200, 0, 0, 20, font, canncel_addc_prompt, NULL, WHITE, edarkgrey, WHITE, NULL);
	tmp_b->text->dst.x -= tmp_b->pos.w + 10;
	tmp_b->text->dst.y -= tmp_b->pos.h + 10;
	tmp_b->pos.x -= tmp_b->pos.w + 10;
	tmp_b->pos.y -= tmp_b->pos.h + 10;
	add_button_to_prompt(add_fan_curve_prompt, tmp_b);

	tmp_b = create_button("no", 0, 1, 300, 200, 0, 0, 20, font, canncel_remove_prompt, NULL, WHITE, edarkgrey, WHITE, NULL);
	tmp_b->text->dst.x -= tmp_b->pos.w + 10;
	tmp_b->text->dst.y -= tmp_b->pos.h + 10;
	tmp_b->pos.x -= tmp_b->pos.w + 10;
	tmp_b->pos.y -= tmp_b->pos.h + 10;
	add_button_to_prompt(remove_curve_prompt, tmp_b);

	add_input_to_prompt(add_fan_curve_prompt, tmp_in);
	add_text_to_prompt(add_fan_curve_prompt, tmp);
	remove_curve_text = create_text("are you sure you want to delete the selected fan curve", 10, 0, 0, 0, 20, remove_curve_prompt->pos.w - 10, WHITE, font, NULL);
	remove_curve_text->dst.y = remove_curve_prompt->pos.h/2 - remove_curve_text->dst.h/2 - 10;
	add_text_to_prompt(remove_curve_prompt, remove_curve_text);
	setting_page_button_f = create_button("settings", 0, 1, 10, 10, 0, 0, 20, font, change_to_settings_page, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);
	rgb_page_button_f = create_button("rgb", 0, 1, 10, setting_page_button_f->pos.y + setting_page_button_f->pos.h + 5,
			0, 0, 20, font, change_to_rgb_page, NULL, WHITE, edarkgrey, WHITE, fan_speed_page);
	rgb_page_button_f->pos.w = setting_page_button_f->pos.w;
	fan_page_button_f = create_button("fan", 0, 1, 10, rgb_page_button_f->pos.y + rgb_page_button_f->pos.h + 5, 
			0, 0, 20, font, change_to_fan_page, NULL, WHITE, grey, BLACK, fan_speed_page);
	fan_page_button_f->pos.w = setting_page_button_f->pos.w;
	blink_cb = create_callback(blink_input, 100.0);
	speed_temp_update_cb = create_callback(update_stuff, 1000/2.0);
	add_callback_to_que(speed_temp_update_cb);
	return 0;
}

void on_remove_prompt_show(void) 
{
	char new_str[300];
	sprintf(new_str, "are you sure you want to delete \"%s\"", fan_curve_arr[fan_curve_ddm->selected_text_index].name);
	int len = strlen(new_str);
	if (new_str[len-2] == '\n') new_str[len-2] = '\"', new_str[len-1] = '\0';

	change_text_and_render_texture(remove_curve_text, new_str, WHITE, font);
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
	rgb_page_button_r = create_button("rgb", 0, 1, 10, setting_page_button_r->pos.y + setting_page_button_r->pos.h + 5,
			0, 0, 20, font, change_to_rgb_page, NULL, WHITE, grey, BLACK, rgb_page);
	rgb_page_button_r->pos.w = setting_page_button_r->pos.w;
	fan_page_button_r = create_button("fan", 0, 1, 10, rgb_page_button_r->pos.y + rgb_page_button_r->pos.h + 5, 
			0, 0, 20, font, change_to_fan_page, NULL, WHITE, edarkgrey, WHITE, rgb_page);
	fan_page_button_r->pos.w = setting_page_button_r->pos.w;

	rgb_mode_ddm = create_drop_down_menu(rgb_modes_amount, tmp_str, 45, fan_page_button_r->pos.y + fan_page_button_r->pos.h + 5, 150, 0, 150, 300, rgb_mode_ddm_select, font, WHITE, edarkgrey, WHITE, rgb_page);
	char str[][MAX_TEXT_SIZE] = { "O", "I", "OI" };
	fan_ring_ddm = create_drop_down_menu(3, str, 10, fan_page_button_r->pos.y + fan_page_button_r->pos.h + 5, 0, 0, 0, 70, fan_ring_select, font, WHITE, edarkgrey, WHITE, rgb_page);

	int prev_w = 0, prev_h = 0;
	for (int i = 0; i < 4; i++) {
		char portstr[7];
		sprintf(portstr, "Port %d", i+1); 
		select_port_rgb_buttons[i] = create_button(portstr, 0, 1, 330 + prev_w, saturation_img->pos.y + saturation_img->pos.h - 73 + prev_h, 0, 0, 0, font, port_select_rgb_func, NULL, WHITE, edarkgrey, WHITE, rgb_page);
		if ((i + 1) % 2 == 0) {
			prev_h = select_port_rgb_buttons[i]->pos.h + 5;
			prev_w = 0;
		} else {
			prev_w = select_port_rgb_buttons[i]->pos.w + 5;
		}
	}
	
	direction_buttons[0] = create_button("<<<", 0, 1, 325, saturation_img->pos.y + saturation_img->pos.h, 0, 0, 40, font, direction_select, NULL, edarkgrey, edarkgrey, WHITE, rgb_page);
	direction_buttons[1] = create_button(">>>", 0, 1, 325 + direction_buttons[0]->pos.w, direction_buttons[0]->pos.y, 0, 0, 40, font, direction_select, NULL, edarkgrey, edarkgrey, WHITE, rgb_page);

	apply_rgb = create_button("Apply", 0, 1, color_img->pos.x, color_buttons[0]->pos.y + color_buttons[0]->pos.h + 10, 0, 0, 0, font, apply, NULL, WHITE, edarkgrey, WHITE, rgb_page);
	apply_to_all_rgb = create_button("Apply all", 0, 1, apply_rgb->pos.x + apply_rgb->pos.w + 5, apply_rgb->pos.y, 0, 0, 0, font, apply, NULL, WHITE, edarkgrey, WHITE, rgb_page);

	rgb_speed_slider = create_slider(1, 330, 200, 200, 10, 20, slider_on_release, rgb_speed_slider_move, WHITE, WHITE, darkgrey, rgb_page);
	rgb_brightnes_slider = create_slider(1, 330, 240, 200, 10, 20, slider_on_release, rgb_brightnes_slider_move, WHITE, WHITE, darkgrey, rgb_page);

	rgb_speed_text = create_text("0%", rgb_speed_slider->pos.x + 210, rgb_speed_slider->button->pos.y - 2, 0, 0, 0, 0, WHITE, font, rgb_page);
	rgb_brightnes_text = create_text("0%", rgb_brightnes_slider->pos.x + 210, rgb_brightnes_slider->button->pos.y - 2, 0, 0, 0 , 0, WHITE, font, rgb_page);

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
	change_text_and_render_texture(rgb_brightnes_text, bs_str, WHITE, font);
	rgb_brightnes_slider->button->pos.x = rgb_brightnes_slider->pos.x + (rgb_brightnes_slider->pos.w * rounded) - rgb_brightnes_slider->button->pos.w/2;

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
	change_text_and_render_texture(rgb_speed_text, bs_str, WHITE, font);
	rgb_speed_slider->button->pos.x = rgb_speed_slider->pos.x + (rgb_speed_slider->pos.w * rounded) - rgb_speed_slider->button->pos.w/2;

	toggle_merge = create_button("Merge", 0, 1, apply_to_all_rgb->pos.x + apply_to_all_rgb->pos.w + 5, apply_to_all_rgb->pos.y, 0, 0, 20, font, toggle_merge_button, NULL, WHITE, edarkgrey, WHITE, rgb_page);

	return 0;
}

struct text *set_fan_count_setting;
struct text *ports_t[4];
int init_settings_page(void)
{
	settings_page = create_page();
	int total_y = 170, total_x = 40;
	for (int p = 0; p < 4; p++) {
		char tmp_str[32];
		sprintf(tmp_str, "%d:", p+1);
		ports_t[p] = create_text(tmp_str, 10, total_y+4, 0, 0, 20, 0, WHITE, font, settings_page);
		for (int f = 0; f < 6; f++) {
			fan_count_buttons[p][f] = create_button(NULL, 0, 1, total_x, total_y, 30, 30, 20, font, fan_count_button_click, NULL, WHITE, BLACK, WHITE, settings_page);
			if (f < ports[p].fan_count) fan_count_buttons[p][f]->bg_color = WHITE;
			total_x += fan_count_buttons[p][f]->pos.w + 5;
		}
		total_x = 40;
		total_y += fan_count_buttons[p][0]->pos.h + 5;
	}
	set_fan_count_setting = create_text("select the amount of fans", 10, 200, 0, 0, 16, 0, WHITE, font, settings_page);
	setting_page_button_s = create_button("settings", 0, 1, 10, 10, 0, 0, 20, font, change_to_settings_page, NULL, WHITE, grey, BLACK, settings_page);
	rgb_page_button_s = create_button("rgb", 0, 1, 10, setting_page_button_s->pos.y + setting_page_button_s->pos.h + 5,
			0, 0, 20, font, change_to_rgb_page, NULL, WHITE, edarkgrey, WHITE, settings_page);
	rgb_page_button_s->pos.w = setting_page_button_s->pos.w;
	fan_page_button_s = create_button("fan", 0, 1, 10, rgb_page_button_s->pos.y + rgb_page_button_s->pos.h + 5, 
			0, 0, 20, font, change_to_fan_page, NULL, WHITE, edarkgrey, WHITE, settings_page);
	fan_page_button_s->pos.w = setting_page_button_s->pos.w;
	set_fan_count_setting->dst.y = fan_page_button_s->pos.y + fan_page_button_s->pos.h + 20;
	return 0;
}

void update_stuff(void)
{
	if (showen_page == fan_speed_page) {
		update_temp();
		update_speed_str();		
	}
	if (showen_page->selected_i != NULL || (showen_prompt != NULL && showen_prompt->selected_input != NULL)) {
		cursor->show = (cursor->show + 1) % 2;
	}
	set_callback_timer(speed_temp_update_cb, 1000/2.0);
}

void blink_input(void)
{
	SDL_Color tmp = { 128, 128, 128, 128 };
	if (blink_cb->times_called_back == 0 || blink_cb->times_called_back == 2) {
		tmp.g = 0; 
		tmp.b = 0;
		render_text_texture(add_fan_curve_prompt->input_arr[0]->default_text, RED, font);
		set_callback_timer(blink_cb, 100.0);
	}
	if (blink_cb->times_called_back == 1 || blink_cb->times_called_back == 3) {
		render_text_texture(add_fan_curve_prompt->input_arr[0]->default_text, tmp, font);
		set_callback_timer(blink_cb, 100.0);
	}
}

void done_prompt(struct button *self, SDL_Event *e)
{
	struct input *i = add_fan_curve_prompt->input_arr[0];
	if (i->text->str[0] == '\0') {
		if (!blink_cb->is_qued)
			add_callback_to_que(blink_cb);
		return;
	}
	add_curve();
	add_item_ddm(fan_curve_ddm, i->text->str, font);
	strcpy(fan_curve_arr[fan_curve_arr_len-1].name, i->text->str);
	select_ddm_item(fan_curve_ddm, fan_curve_ddm->items-1);
	select_curve_ddm(fan_curve_ddm, e);
	strcpy(i->text->str, "");
	i->default_text->show = 1;
	i->text->show = 0;
	cursor->show = 0;
	change_graph_points(fan_curve_graph, fan_curve_arr[fan_curve_ddm->items-1].curve, fan_curve_arr[fan_curve_ddm->items-1].used_points);
	show_prompt(NULL);
}

void canncel_addc_prompt(struct button *self, SDL_Event *e)
{
	struct input *i = add_fan_curve_prompt->input_arr[0];
	strcpy(i->text->str, "");
	add_fan_curve_prompt->input_arr[0]->default_text->show = 1;
	add_fan_curve_prompt->input_arr[0]->text->show = 0;
	cursor->show = 0;
	show_prompt(NULL);
}

void add_curve_bf(struct button *self, SDL_Event *e)
{
	add_fan_curve_prompt->text_arr[0]->show = 1;
	show_prompt(add_fan_curve_prompt);
}

void yes_remove_prompt(struct button *self, SDL_Event *e)
{
	int si = ports[selected_port].curve_i;
	char path[MAX_TEXT_SIZE];
	sprintf(path, "%s%s%d", getenv("HOME"), FAN_CURVE_CONFIG_PATH, si);
	remove(path);
	remove_curve(si);
	remove_item_ddm(fan_curve_ddm, si);
	ports[selected_port].curve_i = 0;
	show_prompt(NULL);
}

void remove_curve_bf(struct button *self, SDL_Event *e)
{
	show_prompt(remove_curve_prompt);
}

void canncel_remove_prompt(struct button *self, SDL_Event *e)
{
	show_prompt(NULL);
}

void select_curve_ddm(struct drop_down_menu *self, SDL_Event *e)
{
	int selected_curve_i = ports[selected_port].curve_i = self->selected_text_index;
	change_graph_points(fan_curve_graph, fan_curve_arr[selected_curve_i].curve, fan_curve_arr[selected_curve_i].used_points);
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

int filter_cpu_input(struct input *self, char new_text[32])
{
	int num = 0, nums[3] = { 0,0,0 };
	int str_l = strlen(self->text->str);
	if (str_l == 1) {
		nums[0] = (self->text->str[0] - 48) * 10;
	} else if (str_l == 2) {
		nums[0] = (self->text->str[0] - 48) * 100;
		nums[1] = (self->text->str[1] - 48) * 10;
	}
	if (new_text[0] >= 48 && new_text[0] <= 57) {
		nums[str_l] = new_text[0] - 48;
	} else return 1;
	num = nums[0] + nums[1] + nums[2];
	if (num > 100) {
		num = 100;
		self->text->str[0] = '1';
		self->text->str[1] = 48;
		new_text[0] = 48;
	}
	if (fan_curve_graph->selected_point != NULL) {
		if (self == cpu_temp_input) fan_curve_graph->selected_point->x = num;
		else if (self == fan_speed_input) fan_curve_graph->selected_point->y = 100-num;
	}
	return 0;
}

void apply_fans_func(struct button *self, SDL_Event *e)
{
	set_fan_curve(&ports[selected_port]);
}

void save_curve_bf(struct button *self, SDL_Event *e) 
{
	int sci = ports[selected_port].curve_i;
	char path[128];
	sprintf(path, "%s%d", FAN_CURVE_CONFIG_PATH, ports[selected_port].curve_i);
	copy_points(fan_curve_arr[sci].curve, &fan_curve_arr[sci].total_points, &fan_curve_arr[sci].used_points, fan_curve_graph->points, fan_curve_graph->point_amount);
	save_curve(fan_curve_arr[sci].curve, fan_curve_arr[sci].name, fan_curve_arr[sci].used_points, path);
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
	sprintf(str, "current cpu temp: %.2f", cpu_temp_num); 
	change_text_and_render_texture(cpu_temp, str, cpu_temp->fg_color, font);
	fan_curve_graph->x.x = cpu_temp_num;
	fclose(fcpu);
}

void moving_graph(struct graph *self, SDL_Event *e)
{
	if (self->selected_point != NULL) {
		char tmp_str[25];
		sprintf(tmp_str, "%d", 100-self->selected_point->y );
		change_input_box_text(fan_speed_input, tmp_str);
		sprintf(tmp_str, "%d", self->selected_point->x); 
		change_input_box_text(cpu_temp_input, tmp_str);
	}
}

void toggle_merge_button(struct button *self, SDL_Event *event)
{
	rgb_merge = (rgb_merge + 1) % 2;
	if (rgb_merge) {
		self->bg_color = WHITE;
		render_text_texture(self->text, BLACK, font);
	} else {
		self->bg_color = edarkgrey;
		render_text_texture(self->text, WHITE, font);
	}

}

void port_select_fan_func(struct button *self, SDL_Event *event) 
{
	for (int i = 0; i < 4; i++) {
		if (self == select_port_fan_buttons[i]) {
			selected_port = i;
			struct curve *selected_curve = &fan_curve_arr[ports[i].curve_i];
			change_graph_points(fan_curve_graph, selected_curve->curve, selected_curve->used_points);
			select_ddm_item(fan_curve_ddm, ports[i].curve_i);
			select_port_fan_buttons[i]->bg_color = WHITE;
			render_text_texture(select_port_fan_buttons[i]->text, BLACK, font);
			char tmp_str[4];
			sprintf(tmp_str, "%d", selected_curve->curve[0].x);
			change_input_box_text(cpu_temp_input, tmp_str);
			uint8_t tmp_speed = 100 - selected_curve->curve[0].y;
			sprintf(tmp_str, "%d", tmp_speed);
			change_input_box_text(fan_speed_input, tmp_str);
		} else {
			select_port_fan_buttons[i]->bg_color = edarkgrey;
			render_text_texture(select_port_fan_buttons[i]->text, WHITE, font);
		}
	}
}
void port_select_rgb_func(struct button *self, SDL_Event *event) 
{
	for (int i = 0; i < 4; i++) {
		if (self == select_port_rgb_buttons[i]) {
			selected_port = i;
			select_port_rgb_buttons[i]->bg_color = WHITE;
			render_text_texture(select_port_rgb_buttons[i]->text, BLACK, font);
		} else {
			select_port_rgb_buttons[i]->bg_color = edarkgrey;
			render_text_texture(select_port_rgb_buttons[i]->text, WHITE, font);
		}
	}
}
void direction_select(struct button *self, SDL_Event *e)
{
	if (self == direction_buttons[0]) {
		direction = 1;
		render_text_texture(direction_buttons[1]->text, WHITE, font);
	} else if (self == direction_buttons[1]) {
		direction = 0;
		render_text_texture(direction_buttons[0]->text, WHITE, font);
	}
	render_text_texture(self->text, BLUE, font);
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
		change_text_and_render_texture(rgb_brightnes_text, tmp_str, WHITE, font);
	}
	self->button->pos.x = self->pos.x + (self->pos.w * rounded) - self->button->pos.w/2;
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
		change_text_and_render_texture(rgb_speed_text, tmp_str, WHITE, font);
	}

	self->button->pos.x = self->pos.x + (self->pos.w * rounded) - self->button->pos.w/2;
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
		get_fan_speed(ports[i].proc_path, &speeds_pro[i], &speeds_rpm[i]);
		sprintf(port_speed_pro[i]->str, "%d%%", speeds_pro[i]);
		sprintf(port_speed_rpm[i]->str, "%d", speeds_rpm[i]);
		change_text_and_render_texture(port_speed_pro[i], port_speed_pro[i]->str, port_speed_pro[i]->fg_color, font);
		change_text_and_render_texture(port_speed_rpm[i], port_speed_rpm[i]->str, port_speed_rpm[i]->fg_color, font);
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

	self->pos.x = mouse_data.x; 

	if (self->pos.x > color_img->pos.x + color_img->pos.w-1) {
		self->pos.x = color_img->pos.x + color_img->pos.w - 1;
	} 
	else if (self->pos.x < color_img->pos.x) {
		self->pos.x = color_img->pos.x;
	} 
	Uint32 *pixels = (Uint32*)color_img->surface->pixels;

	Uint32 pixel = pixels[((self->pos.y - color_img->pos.y)*color_img->surface->h/color_img->pos.h) * 
					color_img->surface->w + ((self->pos.x - color_img->pos.x) * color_img->surface->w/color_img->pos.w)];

	self->bg_color.r = pixel >> 16;
	self->bg_color.g = pixel >> 8;
	self->bg_color.b = pixel;
	self->pos.x -= self->pos.w/2;

	change = 1;
}

void rgb_color_picker_button(struct button *self, SDL_Event *event)
{
	SDL_MouseMotionEvent mouse_data = event->motion;

	self->pos.x = mouse_data.x; 
	self->pos.y = mouse_data.y; 

	if (self->pos.x > saturation_img->pos.x + saturation_img->pos.w) {
		self->pos.x = saturation_img->pos.x + saturation_img->pos.w;
	} else if (self->pos.x < saturation_img->pos.x) {
		self->pos.x = saturation_img->pos.x;
	} 
	if (self->pos.y > saturation_img->pos.y + saturation_img->pos.h) {
		self->pos.y = saturation_img->pos.y + saturation_img->pos.h;
	} else if (self->pos.y < saturation_img->pos.y) {
		self->pos.y = saturation_img->pos.y;
	} 
	int index_y = ((self->pos.y - saturation_img->pos.y) * saturation_img->surface->h / saturation_img->pos.h)-1;
	index_y = index_y < 0 ? 0 : index_y;
	int index_x = ((self->pos.x - saturation_img->pos.x) * saturation_img->surface->w / saturation_img->pos.w)-1;
	index_x = index_x < 0 ? 0 : index_x;
	other_index = index_y * saturation_img->surface->w + index_x;

	Uint32 *pixels = (Uint32*)saturation_img->surface->pixels;
	Uint32 pixel = pixels[other_index];

	self->bg_color.r = pixel >> 16;
	self->bg_color.g = pixel >> 8;
	self->bg_color.b = pixel;
	self->pos.x -= self->pos.w/2;
	self->pos.y -= self->pos.h/2;

	//char new_text[4];
	//sprintf(new_text, "%d", black_white_selector->bg_color.r);
	//change_input_box_text(color_input_r, new_text);
	//sprintf(new_text, "%d", black_white_selector->bg_color.g);
	//change_input_box_text(color_input_g, new_text);
	//sprintf(new_text, "%d", black_white_selector->bg_color.b);
	//change_input_box_text(color_input_b, new_text);
	
}
