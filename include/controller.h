#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <stdio.h>

#include "ui.h"

#ifdef ON_MAC
/* 	TODO paths are worng should take 2 minutes to fix correct path for mac are on macbook 	*/
#define HUB_PATH "../Lian_li_hub/"
#define MBSYNC_PATH "../Lian_li_hub/mb_sync"
#define PORT_ONE_PATH "../Lian_li_hub/Port_one"
#define PORT_TWO_PATH "../Lian_li_hub/Port_two"
#define PORT_THREE_PATH "../Lian_li_hub/Port_three"
#define PORT_FOUR_PATH "../Lian_li_hub/Port_four"
#else
#define HUB_PATH "/proc/Lian_li_hub/"
#define MBSYNC_PATH "/proc/Lian_li_hub/mb_sync"
#define PORT_ONE_PATH "/proc/Lian_li_hub/Port_one"
#define PORT_TWO_PATH "/proc/Lian_li_hub/Port_two"
#define PORT_THREE_PATH "/proc/Lian_li_hub/Port_three"
#define PORT_FOUR_PATH "/proc/Lian_li_hub/Port_four"
#endif

#define INNER 		00001
#define OUTER 		00010
#define INNER_OR_OUTER 	00100
#define MERGE 		01000

struct color {
	Uint8 r, g, b;
};

struct rgb_mode {
	char name[MAX_TEXT_SIZE];
	Uint8 mode; 
	int colors, outerorinner;
};
#include "rgbmodes.h"

struct rgb_data {
	const struct rgb_mode *inner_mode;
	int inner_speed, inner_brightnes, inner_direction;
	struct color inner_color[32];
	const struct rgb_mode *outer_mode;
	int outer_speed, outer_brightnes, outer_direction;
	struct color outer_color[48];
};

struct port {
	const char path[MAX_TEXT_SIZE];
	struct rgb_data rgb;
	int fan_count, fan_speed;
};

int mb_sync = 0;
struct port ports[4] = { 
		{ PORT_ONE_PATH,    { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } }, 4, 30 },
		{ PORT_TWO_PATH,    { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } }, 3, 30 },
		{ PORT_THREE_PATH,  { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } }, 3, 40 },
		{ PORT_FOUR_PATH,   { &rgb_modes[0], 0x02, 0, 0, { 0 }, &rgb_modes[0], 0x02, 0, 0, { 0 } }, 0,  0 }, };


int set_fan_speed(struct port *p, int speed);
/* new_colors lenght should be  96. */
int set_inner_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int brightnes, int direction, struct color *new_colors);
/* new_colors lenght should be 144. */
int set_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, struct color *new_colors);
int set_mb_sync(int state);

#endif
