#include "../include/controller.h"

int get_fan_speed_rpm(const char *p)
{
	char path[MAX_TEXT_SIZE];

	strcpy(path, p);
	strcat(path, "/fan_speed");
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		printf("get_fan_speed_pro failed to open file at %s\n", path);
		return -1;
	}
	int speed_pro, speed_rpm;
	fscanf(f, "%d %d", &speed_pro, &speed_rpm);
	fclose(f);

	return speed_rpm;
}

int get_fan_speed_pro(const char *p)
{
	char path[MAX_TEXT_SIZE];

	strcpy(path, p);
	strcat(path, "/fan_speed");
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		printf("get_fan_speed_pro failed to open file at %s\n", path);
		return -1;
	}
	int speed_pro, speed_rpm;
	fscanf(f, "%d %d", &speed_pro, &speed_rpm);
	fclose(f);

	return speed_pro;
}

/* 
 * 	load inner_mode, inner_speed, inner_brightnes, inner_direction, inner_color, 
 *           outer_mode, outer_speed, outer_brightnes, outer_direction, outer_color,
 *           fan_curve, fan_count
 */
int load_port(struct port *p)
{
	char path[MAX_TEXT_SIZE];

	const char *home_path = getenv("HOME");

	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/fan_count"); 

	FILE *f = fopen(path, "r");
	if (f == NULL) {
		printf("save_port failed to open file at path %s\n", path);
		return -1;
	}
	fscanf(f, "%d", &p->fan_count);
	fclose(f);

	strcpy(path, p->config_path); 
	strcat(path, "/fan_curve"); 
	if (load_graph(p->fan_curve, path) != 0) {
		printf("load failed to save fan_curve graph\n");
		return -1;
	}

	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/inner_rgb"); 

	f = fopen(path, "r");
	int rgb_mode;
	fscanf(f, "%d %d %d %d", &rgb_mode, &p->rgb.inner_speed, &p->rgb.inner_brightnes, &p->rgb.inner_direction);
	p->rgb.inner_mode = &rgb_modes[rgb_mode];

	fclose(f);
	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/inner_colors");

	f = fopen(path, "r");
	size_t n = sizeof(char) * 351;
	char line[351];
	fread(line, sizeof(char), 351, f);
	int stri = 0;
	for (int i = 0; i < 48; i++) {
		unsigned int tmpr, tmpg, tmpb;
		sscanf(&line[stri], "%02x%02x%02x", &tmpr, &tmpb, &tmpg);
		p->rgb.inner_color[i].r = tmpr;
		p->rgb.inner_color[i].g = tmpg;
		p->rgb.inner_color[i].b = tmpb;
		stri += 6;
	}

	fclose(f);

	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/outer_rgb"); 

	f = fopen(path, "w");
	printf("fopen at path %s\n", path);
	fprintf(f, "%d %d %d %d", rgb_mode, p->rgb.outer_speed, p->rgb.outer_brightnes, p->rgb.outer_direction);
	p->rgb.outer_mode = &rgb_modes[rgb_mode];

	fclose(f);
	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/outer_colors");

	f = fopen(path, "r");
	fread(line, sizeof(char), 351, f);
	stri = 0;
	for (int i = 0; i < 72; i++) {
		unsigned int tmpr, tmpg, tmpb;
		sscanf(&line[stri], "%02x%02x%02x", &tmpr, &tmpb, &tmpg);
		p->rgb.outer_color[i].r = tmpr;
		p->rgb.outer_color[i].g = tmpg;
		p->rgb.outer_color[i].b = tmpb;
		stri += 6;
	}

	return 0;
}
int save_port(struct port *p)
{
	char path[MAX_TEXT_SIZE];

	const char *home_path = getenv("HOME");

	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/fan_count"); 

	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("save_port failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d", p->fan_count);
	fclose(f);
	printf("save_port: saved p->fan_count\n");

	strcpy(path, p->config_path); 
	strcat(path, "/fan_curve"); 
	if (save_graph(p->fan_curve, path) != 0) {
		printf("save_port failed to save fan_curve graph\n");
		return -1;
	}
	printf("save_port: saved p->fan_curve\n");

	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/inner_rgb"); 

	f = fopen(path, "w");
	fprintf(f, "%d %d %d %d", p->rgb.inner_mode->index, p->rgb.inner_speed, p->rgb.inner_brightnes, p->rgb.inner_direction);
	printf("save_port: saved p->rgb inner %d %d %d etc.\n", p->rgb.inner_speed, p->rgb.inner_direction, p->rgb.inner_brightnes);

	fclose(f);
	strcpy(path, home_path);
	strcat(path, p->config_path); 
	
	write_inner_colors(path, p->rgb.inner_color, p->fan_count, 1, 0);
	printf("save_port: saved p->rgb.inner_color\n");

	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/outer_rgb"); 

	f = fopen(path, "w");
	fprintf(f, "%d %d %d %d", p->rgb.outer_mode->index, p->rgb.outer_speed, p->rgb.outer_brightnes, p->rgb.outer_direction);
	printf("save_port: saved p->rgb outer %d %d %d etc.\n", p->rgb.inner_speed, p->rgb.inner_direction, p->rgb.inner_brightnes);

	fclose(f);
	strcpy(path, home_path);
	strcat(path, p->config_path); 
	
	write_outer_colors(path, p->rgb.outer_color, p->fan_count, 1, 0);
	printf("save_port: saved p->rgb.outer_color\n");

	return 0;
}

int load_graph(struct graph *g, char *path)
{
	const char *home_path = getenv("HOME");
	if (home_path == NULL) {
		printf("load_graph failed to get home_path\n");
		return -1;
	}
	char new_path[100];
	strcpy(new_path, home_path);
	strcat(new_path, path);
	FILE *f = fopen(new_path, "r");
	if (f == NULL) {
		printf("load_graph failed to open file at path %s\n", new_path);
		return -1;
	}
	char *line = malloc(sizeof(char)*100);
	size_t n = sizeof(char)*100;
	int fan_speed, ct;

	int i = 0;
	while (getline(&line, &n, f) != -1) {
		sscanf(line, "%d %d", &fan_speed, &ct);
		if (g->point_amount + 1 > g->total_points) {
			g->total_points += 5;
			g->points = realloc(g->points, sizeof(struct point) * g->total_points);
		}
		g->points[i].x = ct, g->points[i].y = fan_speed;
		g->point_amount = ++i;

	}
	free(line);
	fclose(f);
	return 0;

}

int save_graph(struct graph *g, char *path)
{
	if (g == NULL) {
		printf("save_graph: error g == NULL\n");
		return -1;
	}
	const char *home_path = getenv("HOME");
	if (home_path == NULL) {
		printf("save_graph failed to get home_path\n");
		return -1;
	}
	char new_path[100];
	strcpy(new_path, home_path);
	strcat(new_path, path);
	FILE *f = fopen(new_path, "w");
	if (f == NULL) {
		printf("save_graph failed to open file at path %s\n", new_path);
		return -1;
	}

	for (int i = 0; i < g->point_amount; i++) {
		fprintf(f, "%d %d\n", g->points[i].y, g->points[i].x);
	}
	fclose(f);
	return 0;
}

float get_fan_speed_from_graph(struct graph *g, float temp) 
{
	for (int i = 0; i < g->point_amount; i++) {
		float xone = (float)g->points[i].x, xtwo = (float)g->points[i + 1].x;
		float yone = 100.0 - g->points[i].y, ytwo = 100.0 - g->points[i + 1].y;
		if (xone < temp && xtwo > temp) {
			float a = (ytwo - yone)/(xtwo - xone);
			float b = yone - a * xone;
			return a * temp + b;
		}
	}
	return 0.0;
}

int set_fan_speed(struct port *p, int speed)
{
	char path[MAX_STR_SIZE];

	strcpy(path, p->proc_path);
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

int set_inner_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_colors, int do_check)
{
	printf("set_inner_rgb\n");
	char path[MAX_STR_SIZE];
	float bright;
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

	strcpy(path, p->proc_path);
	int fan_c = p->fan_count;
	if (set_all) for (int i = 0; i < 4; i++) fan_c = ports[i].fan_count > fan_c ? ports[i].fan_count : fan_c;
	write_inner_colors(path, new_colors, fan_c, bright, new_mode->flags);
	if (do_check && p->rgb.inner_mode == p->rgb.outer_mode && p->rgb.outer_mode->flags & INNER_AND_OUTER && (p->rgb.outer_mode->flags & (INNER | OUTER)) == 0) {
		if (prev_outer_set_all[p->number]) {
			set_outer_rgb(p, prev_rgb_data[p->number].outer_mode, prev_rgb_data[p->number].outer_speed, 
				prev_rgb_data[p->number].outer_direction, prev_rgb_data[p->number].outer_brightnes, 1, prev_rgb_data[p->number].outer_color, 0);
		} else {
			for (int i = 0; i < 4; i++) {
				set_outer_rgb(&ports[i], prev_rgb_data[i].outer_mode, prev_rgb_data[i].outer_speed, 
					prev_rgb_data[i].outer_direction, prev_rgb_data[i].outer_brightnes, 0, prev_rgb_data[i].outer_color, 0);
			}
		}
	}
	strcpy(path, p->proc_path);
	strcat(path, "/inner_rgb");

	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_rgb failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d %d %d %d %d %d", new_mode->mode, speed, direction, brightnes, new_mode->flags, set_all);
	fclose(f);
	if (set_all) {
		for (int i = 0; i < 4; i++) {
			prev_inner_set_all[i] = set_all;
			prev_rgb_data[i].inner_brightnes = ports[i].rgb.inner_brightnes = brightnes;
			prev_rgb_data[i].inner_direction = ports[i].rgb.inner_direction = direction;
			prev_rgb_data[i].inner_speed     = ports[i].rgb.inner_speed     = speed;
			prev_rgb_data[i].inner_mode      = ports[i].rgb.inner_mode      = new_mode;
			memcpy(ports[i].rgb.inner_color, new_colors, sizeof(struct color) * 32);
			memcpy(prev_rgb_data[i].inner_color, new_colors, sizeof(struct color) * 32);
		}
	} else {
		prev_inner_set_all[p->number] = set_all;
		prev_rgb_data[p->number].inner_brightnes = p->rgb.inner_brightnes = brightnes;
		prev_rgb_data[p->number].inner_direction = p->rgb.inner_direction = direction;
		prev_rgb_data[p->number].inner_speed     = p->rgb.inner_speed     = speed;
		prev_rgb_data[p->number].inner_mode      = p->rgb.inner_mode      = new_mode;
		memcpy(p->rgb.inner_color, new_colors, sizeof(struct color) * 32);
		memcpy(prev_rgb_data[p->number].inner_color, new_colors, sizeof(struct color) * 32);
	}
	return 0;
}

/* TODO better remove do_check since its just a tmp fix */
int set_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_colors, int do_check)
{
	printf("set_outer_rgb:\n");
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
	strcpy(path, p->proc_path);

	int fan_c = p->fan_count;
	if (set_all) {
		for (int i = 0; i < 4; i++) {
			fan_c = ports[i].fan_count > fan_c ? ports[i].fan_count : fan_c;
		}
	}

	if (do_check && p->rgb.inner_mode == p->rgb.outer_mode && p->rgb.inner_mode->flags & INNER_AND_OUTER && (p->rgb.inner_mode->flags & (INNER | OUTER)) == 0) {
		if (prev_inner_set_all[p->number]) {
			set_inner_rgb(p, prev_rgb_data[p->number].inner_mode, prev_rgb_data[p->number].inner_speed, 
				prev_rgb_data[p->number].inner_direction, prev_rgb_data[p->number].inner_brightnes, 1, prev_rgb_data[p->number].inner_color, 0);
		} else {
			for (int i = 0; i < 4; i++) {
				set_inner_rgb(&ports[i], prev_rgb_data[i].inner_mode, prev_rgb_data[i].inner_speed, 
					prev_rgb_data[i].inner_direction, prev_rgb_data[i].inner_brightnes, 0, prev_rgb_data[i].inner_color, 0);
			}
		}
	}
	write_outer_colors(path, new_colors, fan_c, bright, new_mode->flags);

	strcpy(path, p->proc_path);
	strcat(path, "/outer_rgb");
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_outer_rgb failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d %d %d %d %d %d", new_mode->mode, speed, direction, brightnes, new_mode->flags, set_all);
	fclose(f);

	if (set_all) {
		for (int i = 0; i < 4; i++) {
			prev_outer_set_all[i] = set_all;
			prev_rgb_data[i].outer_brightnes = ports[i].rgb.outer_brightnes = brightnes;
			prev_rgb_data[i].outer_direction = ports[i].rgb.outer_direction = direction;
			prev_rgb_data[i].outer_speed     = ports[i].rgb.outer_speed     = speed;
			prev_rgb_data[i].outer_mode      = ports[i].rgb.outer_mode      = new_mode;
			memcpy(ports[i].rgb.outer_color, new_colors, sizeof(struct color) * 48);
			memcpy(prev_rgb_data[i].outer_color, new_colors, sizeof(struct color) * 48);
		}
	} else {
		prev_outer_set_all[p->number] = set_all;
		prev_rgb_data[p->number].outer_brightnes = p->rgb.outer_brightnes = brightnes;
		prev_rgb_data[p->number].outer_direction = p->rgb.outer_direction = direction;
		prev_rgb_data[p->number].outer_speed     = p->rgb.outer_speed     = speed;
		prev_rgb_data[p->number].outer_mode      = p->rgb.outer_mode      = new_mode;
		memcpy(p->rgb.outer_color, new_colors, sizeof(struct color) * 48);
		memcpy(prev_rgb_data[p->number].outer_color, new_colors, sizeof(struct color) * 48);
	}
	return 0;
}

int set_inner_and_outer_rgb(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, int set_all, struct color *new_outer_colors, struct color *new_inner_colors)
{

	printf("set_inner_and_outer_rgb\n");
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
	strcpy(path, p->proc_path);
	int fan_c = p->fan_count;
	int flag = new_mode->flags - (new_mode->flags & MERGE);
	if (set_all) for (int i = 0; i < 4; i++) fan_c = ports[i].fan_count > fan_c ? ports[i].fan_count : fan_c;
	write_outer_colors(path, new_outer_colors, fan_c, bright, flag);

	strcpy(path, p->proc_path);
	write_inner_colors(path, new_inner_colors, fan_c, bright, flag);

	strcpy(path, p->proc_path);
	strcat(path, "/inner_and_outer_rgb");
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_inner_and_outer_color failed to open file at path %s\n", path);
		return -1;
	}

	fprintf(f, "%d %d %d %d %d %d", new_mode->mode, speed, direction, brightnes, flag, set_all);
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

int set_merge(struct port *p, const struct rgb_mode *new_mode, int speed, int direction, int brightnes, struct color *new_colors)
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
	strcpy(path, p->proc_path);
	write_inner_colors(path, new_colors, p->fan_count, bright, new_mode->flags);

	strcpy(path, p->proc_path);
	strcat(path, "/inner_and_outer_rgb");
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_merge failed to open file at path %s\n", path);
		return -1;
	}
	fprintf(f, "%d %d %d %d %d %d", new_mode->merge_mode, speed, direction, brightnes, new_mode->flags, 1); 
	fclose(f);
	for (int i = 0; i < 4; i++) {
		ports[i].rgb.inner_brightnes = ports[i].rgb.outer_brightnes = brightnes;
		ports[i].rgb.inner_direction = ports[i].rgb.outer_direction = direction;
		ports[i].rgb.inner_speed     = ports[i].rgb.outer_speed     = speed;
		ports[i].rgb.inner_mode      = ports[i].rgb.outer_mode      = new_mode;
		memcpy(ports[i].rgb.inner_color, new_colors, sizeof(struct color) * 32);
	}
	return 0;
}

int write_outer_colors(char *path, struct color *new_colors, int fan_count, float bright, int flags)
{

	strcat(path, "/outer_colors");

	FILE *f = fopen(path, "w");
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
		
		str_i += 6;
	}
	fputs(inner_color_str, f);
	fclose(f);
	return 0;
}





