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

int set_inner_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int brightnes, int direction, struct color *new_colors)
{
	char path[MAX_STR_SIZE];

	strcpy(path, p->path);
	strcat(path, "/inner_colors");

	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_rgb failed to open file at path %s\n", path);
		return -1;
	}

	int str_i = 0;
	char color_str[192];
	for (int i = 0; i < 8 * p->fan_count; i++) {
		sprintf(&color_str[str_i], "%02x%02x%02x", new_colors[i].r, new_colors[i].b, new_colors[i].g);
		str_i += 6;
	}
	fputs(color_str, f);
	fclose(f);
	memcpy(p->rgb.inner_color, new_colors, sizeof(struct color) * 32);

	strcpy(path, p->path);
	strcat(path, "/inner_rgb");

	f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_rgb failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d %d %d %d", new_mode->mode, speed, direction, brightnes);
	printf("%d %d %d %d", new_mode->mode, speed, direction, brightnes);
	fclose(f);

	p->rgb.inner_brightnes = brightnes;
	p->rgb.inner_direction = direction;
	p->rgb.inner_speed     = speed;
	p->rgb.inner_mode      = new_mode;

	return 0;
}

int set_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, struct color *new_colors)
{
	char path[MAX_STR_SIZE];

	strcpy(path, p->path);
	strcat(path, "/outer_colors");

	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_outer_rgb failed to open file at path %s\n", path);
		return -1;
	}

	int str_i = 0;
	char color_str[288];
	for (int i = 0; i < 12 * p->fan_count; i++) {
		sprintf(&color_str[str_i], "%02x%02x%02x", new_colors[i].r, new_colors[i].b, new_colors[i].g);
		str_i += 6;
	}
	fputs(color_str, f);
	fclose(f);
	memcpy(p->rgb.outer_color, new_colors, sizeof(struct color) * 48);

	strcpy(path, p->path);
	strcat(path, "/outer_rgb");
	f = fopen(path, "w");
	if (f == NULL) {
		printf("set_outer_rgb failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d %d %d %d", new_mode->mode, speed, direction, brightnes);
	printf("%d %d %d %d", new_mode->mode, speed, direction, brightnes);
	fclose(f);

	p->rgb.outer_brightnes = brightnes;
	p->rgb.outer_direction = direction;
	p->rgb.outer_speed     = speed;
	p->rgb.outer_mode      = new_mode;
	return 0;
}

int set_inner_and_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, struct color *new_outer_colors, struct color *new_inner_colors)
{
	char path[MAX_STR_SIZE];

	strcpy(path, p->path);
	strcat(path, "/outer_colors");

	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_and_outer_color failed to open file at path %s\n", path);
		return -1;
	}

	int str_i = 0;
	char outer_color_str[288];
	for (int i = 0; i < 12 * p->fan_count; i++) {
		sprintf(&outer_color_str[str_i], "%02x%02x%02x", new_outer_colors[i].r, new_outer_colors[i].b, new_outer_colors[i].g);
		str_i += 6;
	}
	fputs(outer_color_str, f);
	fclose(f);
	memcpy(p->rgb.outer_color, new_outer_colors, sizeof(struct color) * 48);

	strcpy(path, p->path);
	strcat(path, "/inner_colors");

	f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_and_outer_rgb failed to open file at path %s\n", path);
		return -1;
	}

	str_i = 0;
	char inner_color_str[192];
	for (int i = 0; i < 8 * p->fan_count; i++) {
		sprintf(&inner_color_str[str_i], "%02x%02x%02x", new_inner_colors[i].r, new_inner_colors[i].b, new_inner_colors[i].g);
		str_i += 6;
	}
	fputs(inner_color_str, f);
	fclose(f);
	memcpy(p->rgb.inner_color, new_inner_colors, sizeof(struct color) * 32);

	strcpy(path, p->path);
	strcat(path, "/inner_and_outer_rgb");
	f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_and_outer_color failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d %d %d %d", new_mode->mode, speed, direction, brightnes);
	fclose(f);
	p->rgb.inner_brightnes = p->rgb.outer_brightnes = brightnes;
	p->rgb.inner_direction = p->rgb.outer_direction = direction;
	p->rgb.inner_speed     = p->rgb.outer_speed     = speed;
	p->rgb.inner_mode      = p->rgb.outer_mode      = new_mode;

	return 0;
}









