#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/ui.h"

#define HUB_PATH "/proc/Lian_li_hub/"
#define MBSYNC_PATH "/proc/Lian_li_hub/mb_sync"
#define PORT_ONE_PATH "/proc/Lian_li_hub/Port_one"
#define PORT_TWO_PATH "/proc/Lian_li_hub/Port_two"
#define PORT_THREE_PATH "/proc/Lian_li_hub/Port_three"
#define PORT_FOUR_PATH "/proc/Lian_li_hub/Port_four"

#define FAN_CURVE_PATH "/.config/Ll-connect-config/fan_curve"
/* 
 * 	save inner_mode, inner_speed, inner_brightnes, inner_direction, inner_color, 
 *           outer_mode, outer_speed, outer_brightnes, outer_direction, outer_color,
 *           fan_curve, fan_count
 */
#define PORT_ONE_CONFIG_PATH "/.config/Ll-connect-config/Port_1"
#define PORT_TWO_CONFIG_PATH "/.config/Ll-connect-config/Port_2"
#define PORT_THREE_CONFIG_PATH "/.config/Ll-connect-config/Port_3"
#define PORT_FOUR_CONFIG_PATH "/.config/Ll-connect-config/Port_4"

#define MAX_STR_SIZE 256

#define INNER 		0b00000001
#define OUTER 		0b00000010
#define INNER_AND_OUTER 0b00000100
#define MERGE 		0b00001000
/* flag for modes where they should send inner and outer color fx. Static Color */
#define NOT_MOVING 	0b00010000

#define BRIGHTNESS 	0b00100000
#define SPEED 		0b01000000
#define DIRECTION 	0b10000000

struct color {
	u_int8_t r, g, b;
};

struct rgb_mode {
	char name[MAX_STR_SIZE];
	u_int8_t mode, merge_mode; 
	int colors, flags, index;
};
#include "rgbmodes.h"

struct rgb_data {
	const struct rgb_mode *inner_mode;
	int inner_speed, inner_brightnes, inner_direction;
	struct color inner_color[48];
	const struct rgb_mode *outer_mode;
	int outer_speed, outer_brightnes, outer_direction;
	struct color outer_color[72];
};

struct port {
	const char proc_path[MAX_STR_SIZE];
	const char config_path[MAX_STR_SIZE];
	struct rgb_data rgb;
	int fan_count, fan_speed, number;
	struct graph *fan_curve;
};

int mb_sync = 0;
struct port ports[4] = { 
		{ PORT_ONE_PATH,   PORT_ONE_CONFIG_PATH,    { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } }, 4, 30, 0 },
		{ PORT_TWO_PATH,   PORT_TWO_CONFIG_PATH,    { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } }, 3, 30, 1 },
		{ PORT_THREE_PATH, PORT_THREE_CONFIG_PATH,  { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } }, 3, 40, 2 },
		{ PORT_FOUR_PATH,  PORT_FOUR_CONFIG_PATH,   { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } }, 0,  0, 3 }, };

int prev_inner_set_all[4];
int prev_outer_set_all[4];
struct rgb_data prev_rgb_data[4] = { { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } },
                                     { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } },
                                     { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } },
                                     { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } } };


int get_fan_speed_rpm(const char *p);
int get_fan_speed_pro(const char *p);
int load_graph(struct graph *g, char *path);
int save_graph(struct graph *g, char *path);
int set_fan_speed(struct port *p, int speed);
int set_inner_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_colors, int do_check);
int set_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_colors, int do_check);
int set_inner_and_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_outer_colors, struct color *new_inner_colors);
int set_merge(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, struct color *new_colors);
int write_outer_colors(char *path, struct color *new_colors, int fan_count, float bright, int flags);
int write_inner_colors(char *path, struct color *new_colors, int fan_count, float bright, int flags);
int set_mb_sync(int state);

#endif
