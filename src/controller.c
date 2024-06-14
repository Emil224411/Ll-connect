#include "../include/controller.h"

int set_fan_speed(struct port *p, int speed)
{
	char path[MAX_STR_SIZE];

	strcpy(path, p->path);
	strcat(path, "/fan_speed");
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_fan_speed failed\n");
		return -1;
	}
	fprintf(f, "%d", speed); 
	fclose(f);
	p->fan_speed = speed;
	return 0;
}

int set_mb_sync(int state) 
{
	FILE *f = fopen(MBSYNC_PATH, "w");
	if (f == NULL) {
		printf("set mb sync failed\n");
		return -1;
	}
	fprintf(f, "%d", state); 
	mb_sync = state;
	fclose(f);
	return 0;
}

int set_inner_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int brightnes, int direction, int set_all, struct color *new_colors)
{
	char path[MAX_STR_SIZE];
	float bright = 0.0;
/*
 *	0%   = 08
 *	25%  = 03
 *	50%  = 02
 *	75%  = 01
 *	100% = 00
 */
	switch (brightnes) {
		case 0x01:
			bright = 0.75;
			break;
		case 0x02:
			bright = 0.50;
			break;
		case 0x03:
			bright = 0.25;
			break;
		case 0x08:
			bright = 0.0;
			break;
		default:
			bright = 1.0;
			break;
	}

	strcpy(path, p->path);

	write_inner_colors(path, new_colors, p->fan_count, bright, new_mode->flags);

	strcpy(path, p->path);
	strcat(path, "/inner_rgb");

	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_rgb failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d %d %d %d %d %d", new_mode->mode, speed, direction, brightnes, new_mode->flags, set_all);
	printf("%d %d %d %d %d\n", new_mode->mode, speed, direction, brightnes, set_all);
	fclose(f);
	if (set_all) {
		for (int i = 0; i < 4; i++) {
			ports[i].rgb.inner_brightnes = brightnes;
			ports[i].rgb.inner_speed = speed;
			ports[i].rgb.inner_direction = direction;
			ports[i].rgb.inner_mode = new_mode;
			memcpy(ports[i].rgb.inner_color, new_colors, sizeof(struct color) * 32);
		}
	} else {
		p->rgb.inner_brightnes = brightnes;
		p->rgb.inner_direction = direction;
		p->rgb.inner_speed     = speed;
		p->rgb.inner_mode      = new_mode;
		memcpy(p->rgb.inner_color, new_colors, sizeof(struct color) * 32);
	}

	return 0;
}

int set_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_colors)
{
	char path[MAX_STR_SIZE];
	float bright;
	switch (brightnes) {
		case 0x01:
			bright = 0.75;
			break;
		case 0x02:
			bright = 0.50;
			break;
		case 0x03:
			bright = 0.25;
			break;
		case 0x08:
			bright = 0.0;
			break;
		default:
			bright = 1.0;
			break;
	}
	strcpy(path, p->path);

	write_outer_colors(path, new_colors, p->fan_count, bright, new_mode->flags);

	strcpy(path, p->path);
	strcat(path, "/outer_rgb");
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_outer_rgb failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d %d %d %d %d %d", new_mode->mode, speed, direction, brightnes, new_mode->flags, set_all);
	printf("%d %d %d %d %d\n", new_mode->mode, speed, direction, brightnes, set_all);
	fclose(f);

	if (set_all) {
		for (int i = 0; i < 4; i++) {
			ports[i].rgb.outer_brightnes = brightnes;
			ports[i].rgb.outer_speed = speed;
			ports[i].rgb.outer_direction = direction;
			ports[i].rgb.outer_mode = new_mode;
			memcpy(ports[i].rgb.outer_color, new_colors, sizeof(struct color) * 32);
		}
	} else {
		p->rgb.outer_brightnes = brightnes;
		p->rgb.outer_direction = direction;
		p->rgb.outer_speed     = speed;
		p->rgb.outer_mode      = new_mode;
		memcpy(p->rgb.outer_color, new_colors, sizeof(struct color) * 48);
	}
	return 0;
}

int set_inner_and_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_outer_colors, struct color *new_inner_colors)
{

	float bright;
	switch (brightnes) {
		case 0x01:
			bright = 0.75;
			break;
		case 0x02:
			bright = 0.50;
			break;
		case 0x03:
			bright = 0.25;
			break;
		case 0x08:
			bright = 0.0;
			break;
		default:
			bright = 1.0;
			break;
	}

	char path[MAX_STR_SIZE];
	strcpy(path, p->path);
	write_outer_colors(path, new_outer_colors, p->fan_count, bright, new_mode->flags);

	strcpy(path, p->path);
	write_inner_colors(path, new_inner_colors, p->fan_count, bright, new_mode->flags);

	strcpy(path, p->path);
	strcat(path, "/inner_and_outer_rgb");
	printf("open file at path %s\n", path);
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_and_outer_color failed to open file at path %s\n", path);
		return -1;
	}

	fprintf(f, "%d %d %d %d %d %d", new_mode->mode, speed, direction, brightnes, new_mode->flags, set_all);
	fclose(f);

	if (set_all) {
		for (int i = 0; i < 4; i++) {
			ports[i].rgb.inner_brightnes = ports[i].rgb.outer_brightnes = brightnes;
			ports[i].rgb.inner_direction = ports[i].rgb.outer_direction = direction;
			ports[i].rgb.inner_speed     = ports[i].rgb.outer_speed     = speed;
			ports[i].rgb.inner_mode      = ports[i].rgb.outer_mode      = new_mode;
			memcpy(ports[i].rgb.inner_color, new_inner_colors, sizeof(struct color) * 32);
			memcpy(ports[i].rgb.outer_color, new_outer_colors, sizeof(struct color) * 48);
		}
	} else {
		p->rgb.inner_brightnes = p->rgb.outer_brightnes = brightnes;
		p->rgb.inner_direction = p->rgb.outer_direction = direction;
		p->rgb.inner_speed     = p->rgb.outer_speed     = speed;
		p->rgb.inner_mode      = p->rgb.outer_mode      = new_mode;
		memcpy(p->rgb.inner_color, new_inner_colors, sizeof(struct color) * 32);
		memcpy(p->rgb.outer_color, new_outer_colors, sizeof(struct color) * 48);
	}

	return 0;
}

int write_outer_colors(char *path, struct color *new_colors, int fan_count, float bright, int flags)
{

	strcat(path, "/outer_colors");

	FILE *f = fopen(path, "w");
	printf("write_outer_colors open file at path %s\n", path);
	if (f == NULL) {
		printf("write_outer_colors failed to open file at path %s\n", path);
		return -1;
	}

	int str_i = 0;
	char outer_color_str[288];
	for (int i = 0; i < 12 * fan_count; i++) {
		int r = new_colors[i].r;
		int g = new_colors[i].g;
		int b = new_colors[i].b;
		if (flags & NOT_MOVING) {
			r *= bright;
			g *= bright;
			b *= bright;
		}
		sprintf(&outer_color_str[str_i], "%02x%02x%02x", r, b, g);
		printf("%02x%02x%02x\n", r, b, g);
		str_i += 6;
	}
	fputs(outer_color_str, f);
	fclose(f);
	return 0;
}

int write_inner_colors(char *path, struct color *new_colors, int fan_count, float bright, int flags)
{

	strcat(path, "/inner_colors");

	FILE *f = fopen(path, "w");
	printf("write_inner_colors open file at path %s\n", path);
	if (f == NULL) {
		printf("write_inner_colors failed to open file at path %s\n", path);
		return -1;
	}

	int str_i = 0;
	char inner_color_str[192];
	for (int i = 0; i < 8 * fan_count; i++) {
		int r = new_colors[i].r;
		int g = new_colors[i].g;
		int b = new_colors[i].b;
		if (flags & NOT_MOVING) {
			r *= bright;
			g *= bright;
			b *= bright;
		}
		sprintf(&inner_color_str[str_i], "%02x%02x%02x", r, b, g);
		printf("%02x%02x%02x\n", r, b, g);
		str_i += 6;
	}
	fputs(inner_color_str, f);
	fclose(f);
	return 0;
}





