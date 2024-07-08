#ifndef CONTROLLER_H
#define CONTROLLER_H
#define _GNU_SOURCE
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

#define HUB_PATH "/proc/Lian_li_hub/"
#define MBSYNC_PATH "/proc/Lian_li_hub/mb_sync"
#define PORT_ONE_PATH "/proc/Lian_li_hub/Port_one"
#define PORT_TWO_PATH "/proc/Lian_li_hub/Port_two"
#define PORT_THREE_PATH "/proc/Lian_li_hub/Port_three"
#define PORT_FOUR_PATH "/proc/Lian_li_hub/Port_four"

#define CONFIG_PATH "/.config/Ll-connect-config"
#define FAN_CURVE_CONFIG_PATH "/.config/Ll-connect-config/fan_curve_"
#define PORT_ONE_CONFIG_PATH "/.config/Ll-connect-config/Port_1"
#define PORT_TWO_CONFIG_PATH "/.config/Ll-connect-config/Port_2"
#define PORT_THREE_CONFIG_PATH "/.config/Ll-connect-config/Port_3"
#define PORT_FOUR_CONFIG_PATH "/.config/Ll-connect-config/Port_4"

/* rgb mode flags */
#define INNER 		0x01
#define OUTER 		0x02
#define INNER_AND_OUTER 0x04
#define MERGE 		0x08
#define NOT_MOVING 	0x10
#define BRIGHTNESS 	0x20
#define SPEED 		0x40
#define DIRECTION 	0x80

struct color {
	u_int8_t r, g, b;
};

struct rgb_mode {
	char name[MAX_TEXT_SIZE];
	u_int8_t mode, merge_mode; 
	int colors, flags, index;
};

struct rgb_data {
	const struct rgb_mode *inner_mode;
	int inner_speed, inner_brightnes, inner_direction;
	struct color inner_color[48];
	const struct rgb_mode *outer_mode;
	int outer_speed, outer_brightnes, outer_direction;
	struct color outer_color[72];
};

struct port {
	const char proc_path[MAX_TEXT_SIZE];
	const char config_path[MAX_TEXT_SIZE];
	struct rgb_data rgb;
	int fan_count, fan_speed, number;
	struct point *curve;
	int curve_i;
	int points_used, points_total;
};

struct curve {
	char name[MAX_TEXT_SIZE];
	int total_points, used_points;
	struct point *curve;
};

/* global varibles */
extern const int rgb_modes_amount;
extern const struct rgb_mode rgb_modes[46];
extern struct port ports[4];
extern int mb_sync;
extern int prev_inner_set_all[4];
extern int prev_outer_set_all[4];
extern struct rgb_data prev_rgb_data[4];

extern struct curve *fan_curve_arr;
extern int fan_curve_arr_len;

int init_controller(void);
void init_fan_curve_conf(void);
void shutdown_controller(void);
/* save/load functions */
int save_port(struct port *p);
int load_port(struct port *p);
int save_curve(struct point *p, char *name, int points_used, char *path);
int load_curve(struct point **p, char *name, int name_len, int *points_used, int *points_total, char *path);
void remove_curve(int index);
void add_curve(void);
struct point *alloc_point_arr(int size);

/* set rgb functions */
int set_inner_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_colors, int do_check);
int set_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_colors, int do_check);
int set_inner_and_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_outer_colors, struct color *new_inner_colors);
int set_merge(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, struct color *new_colors);

/* color functions */
int write_outer_colors(char *path, struct color *new_colors, int fan_count, float bright, int flags);
int write_inner_colors(char *path, struct color *new_colors, int fan_count, float bright, int flags);

/* set fan speed functions */
int set_fan_curve(struct port *p);
int set_fan_speed(struct port *p, int speed);
int get_fan_speed_rpm(const char *path);
int get_fan_speed_pro(const char *path);

int set_mb_sync(int state);
int get_fan_count_from_driver(struct port *p);
int set_fan_count(struct port *p, int fc);

#endif
