#include "../include/controller.h"

int set_fan_speed(struct port *p, int speed)
{
	char path[MAX_TEXT_SIZE];

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
	char path[MAX_TEXT_SIZE];

	strcpy(path, p->path);
	strcat(path, "/inner_colors");
	printf("file at path %s\n", path);

	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_rgb failed to open file at path %s\n", path);
		return -1;
	}

	int str_i = 0;
	char color_str[192];
	for (int i = 0; i < 32; i++) {
		sprintf(&color_str[str_i], "%02x%02x%02x", new_colors[i].r, new_colors[i].g, new_colors[i].b);
		str_i += 6;
	}
	fputs(color_str, f);
	fclose(f);
	memcpy(p->rgb.inner_color, new_colors, sizeof(struct color) * 32);

	strcpy(path, p->path);
	strcat(path, "/inner_rgb");
	printf("file at path %s\n", path);

	f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_rgb failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d %d %d %d", new_mode->mode, speed, direction, brightnes);
	fclose(f);

	p->rgb.inner_brightnes = brightnes;
	p->rgb.inner_direction = direction;
	p->rgb.inner_speed     = speed;
	p->rgb.inner_mode      = new_mode;

	return 0;
}

int set_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, struct color *new_colors)
{
	char path[MAX_TEXT_SIZE];

	strcpy(path, p->path);
	strcat(path, "/outer_colors");
	printf("file at path %s\n", path);

	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_outer_rgb failed to open file at path %s\n", path);
		return -1;
	}

	int str_i = 0;
	char color_str[288];
	for (int i = 0; i < 48; i++) {
		sprintf(&color_str[str_i], "%02x%02x%02x", new_colors[i].r, new_colors[i].g, new_colors[i].b);
		//printf("i = %d %02x, %02x, %02x\n", i, new_colors[i].r, new_colors[i].g, new_colors[i].b);
		str_i += 6;
	}
	fputs(color_str, f);
	fclose(f);
	memcpy(p->rgb.inner_color, new_colors, sizeof(struct color) * 48);

	strcpy(path, p->path);
	strcat(path, "/outer_rgb");
	printf("file at path %s\n", path);
	f = fopen(path, "w");
	if (f == NULL) {
		printf("set_outer_rgb failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d %d %d %d", new_mode->mode, speed, direction, brightnes);
	fclose(f);

	p->rgb.inner_brightnes = brightnes;
	p->rgb.inner_direction = direction;
	p->rgb.inner_speed     = speed;
	p->rgb.inner_mode      = new_mode;

	return 0;
}

int set_inner_and_outer_color()
{
	
	return 0;
}
