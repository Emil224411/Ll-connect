#include "controller.h"

const struct rgb_mode rgb_modes[] = {
	{ "Static Color",      0x01, 0x0,  6, INNER | OUTER | INNER_AND_OUTER | NOT_MOVING | BRIGHTNESS, 0 }, 
	{ "Breathing",         0x02, 0x0,  6, INNER | OUTER | INNER_AND_OUTER | NOT_MOVING | BRIGHTNESS | SPEED, 1 },
	{ "Rainbow Morph",     0x04, 0x0,  0, INNER | OUTER | INNER_AND_OUTER | BRIGHTNESS | SPEED, 2 },
	{ "Rainbow",           0x05, 0x0,  0, INNER | OUTER | BRIGHTNESS | SPEED | DIRECTION, 3 },
	{ "Breathing Rainbow", 0x06, 0x0,  0, OUTER | BRIGHTNESS | SPEED, 4 },
	{ "Meteor Rainbow",    0x08, 0x0,  0, INNER | OUTER | BRIGHTNESS | SPEED, 5 },
	{ "Color Cycle",       0x18, 0x0,  4, INNER | BRIGHTNESS | SPEED | DIRECTION, 6 },
	{ "Meteor", 	       0x19, 0x0,  4, INNER | OUTER | INNER_AND_OUTER | BRIGHTNESS | SPEED | DIRECTION, 7 },
	{ "Runway", 	       0x1a, 0x46, 2, INNER | OUTER | INNER_AND_OUTER | MERGE | BRIGHTNESS | SPEED | DIRECTION, 8 },
	{ "Mop Up", 	       0x1b, 0x0,  2, INNER | OUTER | SPEED | BRIGHTNESS, 9 },
	{ "Color Cycle",       0x1c, 0x0,  4, OUTER | BRIGHTNESS | SPEED | DIRECTION, 10 }, 
	{ "Lottery", 	       0x1d, 0x0,  2, INNER | OUTER  | BRIGHTNESS | SPEED | DIRECTION, 11 },
	{ "Wave", 	       0x1e, 0x0,  1, INNER | OUTER  | BRIGHTNESS | SPEED, 12 }, 
	{ "Spring", 	       0x1f, 0x0,  4, INNER | OUTER  | BRIGHTNESS | SPEED | DIRECTION, 13 },
	{ "Tail Chasing",      0x20, 0x0,  4, INNER | OUTER  | BRIGHTNESS | SPEED | DIRECTION, 14 },
	{ "Warning", 	       0x21, 0x0,  4, INNER | OUTER  | BRIGHTNESS | SPEED, 15 },
	{ "Voice", 	       0x22, 0x0,  4, INNER | OUTER  | BRIGHTNESS | SPEED | DIRECTION, 16 },
	{ "Mixing", 	       0x23, 0x0,  2, INNER | OUTER | BRIGHTNESS | SPEED, 17 },
	{ "Stack", 	       0x24, 0x0,  2, INNER | OUTER | BRIGHTNESS | SPEED | DIRECTION, 18 },
	{ "Tide", 	       0x25, 0x0,  4, INNER | OUTER | BRIGHTNESS | SPEED, 19 },
	{ "Scan", 	       0x26, 0x0,  1, INNER | OUTER | BRIGHTNESS | SPEED, 20 },
	{ "Pac-Man", 	       0x27, 0x0,  2, INNER | BRIGHTNESS | SPEED | DIRECTION, 21 },
	{ "Colorful City",     0x28, 0x0,  0, INNER | OUTER | BRIGHTNESS | SPEED, 22 },
	{ "Render", 	       0x29, 0x0,  4, INNER | OUTER | BRIGHTNESS | SPEED | DIRECTION, 23 },
	{ "Twinkle", 	       0x2a, 0x0,  0, INNER | OUTER | BRIGHTNESS | SPEED, 24 },
	{ "Rainbow", 	       0x2b, 0x0,  0, INNER_AND_OUTER | BRIGHTNESS | SPEED | DIRECTION, 25 },
	{ "Color Cycle",       0x2e, 0x0,  4, INNER_AND_OUTER | BRIGHTNESS | SPEED | DIRECTION, 26 },
	{ "Taichi", 	       0x2f, 0x0,  2, INNER_AND_OUTER | BRIGHTNESS | SPEED | DIRECTION, 27 },
	{ "Warning", 	       0x30, 0x0,  4, INNER_AND_OUTER | BRIGHTNESS | SPEED, 28 },
	{ "Voice", 	       0x31, 0x0,  4, INNER_AND_OUTER | BRIGHTNESS | SPEED | DIRECTION, 29 },
	{ "Mixing", 	       0x32, 0x47, 2, INNER_AND_OUTER | MERGE | BRIGHTNESS | SPEED, 30 },
	{ "Tide", 	       0x33, 0x48, 4, INNER_AND_OUTER | MERGE | BRIGHTNESS | SPEED, 31 },
	{ "Scan", 	       0x34, 0x44, 2, INNER_AND_OUTER | MERGE | BRIGHTNESS | SPEED, 32 },
	{ "Contest", 	       0x35, 0x45, 3, INNER_AND_OUTER | MERGE | BRIGHTNESS | SPEED | DIRECTION, 33 },
	{ "Colorful City",     0x38, 0x0,  0, INNER_AND_OUTER | BRIGHTNESS | SPEED, 34 },
	{ "Render", 	       0x39, 0x0,  4, INNER_AND_OUTER | BRIGHTNESS | SPEED | DIRECTION, 35 },
	{ "Twinkle", 	       0x3a, 0x0,  0, INNER_AND_OUTER | BRIGHTNESS | SPEED, 36 },
	{ "Wave", 	       0x3b, 0x49, 1, INNER_AND_OUTER | MERGE | BRIGHTNESS | SPEED, 37 },
	{ "Spring", 	       0x3c, 0x4b, 4, INNER_AND_OUTER | MERGE | BRIGHTNESS | SPEED | DIRECTION, 38 },
	{ "Tail Chasing",      0x3d, 0x4a, 4, INNER_AND_OUTER | MERGE | BRIGHTNESS | SPEED | DIRECTION, 39 },
	{ "Mop Up", 	       0x3e, 0x4c, 2, INNER_AND_OUTER | MERGE | BRIGHTNESS | SPEED, 40 },
	{ "Tornado", 	       0x3f, 0x0,  4, INNER_AND_OUTER | BRIGHTNESS | SPEED | DIRECTION, 41 },
	{ "Staggered", 	       0x40, 0x0,  4, INNER_AND_OUTER | BRIGHTNESS | SPEED, 42 },
	{ "Spanning Teacups",  0x41, 0x0,  4, INNER_AND_OUTER | BRIGHTNESS | SPEED | DIRECTION, 43 },
	{ "Electric Current",  0x42, 0x4f, 4, INNER_AND_OUTER | MERGE | BRIGHTNESS | SPEED, 44 },
	{ "Stack", 	       0x43, 0x0,  2, INNER_AND_OUTER | BRIGHTNESS | SPEED | DIRECTION, 45 }, 
};
const int rgb_modes_amount = 46;

int mb_sync = 0;

int prev_inner_set_all[4];
int prev_outer_set_all[4];
struct rgb_data prev_rgb_data[4] = { 
	{ &rgb_modes[0], 0x02, 0, 0, { {0} }, &rgb_modes[0], 0x02, 0, 0, { {0} } },
	{ &rgb_modes[0], 0x02, 0, 0, { {0} }, &rgb_modes[0], 0x02, 0, 0, { {0} } },
	{ &rgb_modes[0], 0x02, 0, 0, { {0} }, &rgb_modes[0], 0x02, 0, 0, { {0} } },
	{ &rgb_modes[0], 0x02, 0, 0, { {0} }, &rgb_modes[0], 0x02, 0, 0, { {0} } }
};

struct port ports[4] = { 
	{ PORT_ONE_PATH,   PORT_ONE_CONFIG_PATH,   .number = 0 },
	{ PORT_TWO_PATH,   PORT_TWO_CONFIG_PATH,   .number = 1 },
	{ PORT_THREE_PATH, PORT_THREE_CONFIG_PATH, .number = 2 },
	{ PORT_FOUR_PATH,  PORT_FOUR_CONFIG_PATH,  .number = 3 }, 
};

struct point default_fan_curve[7] = {
	{ 10, 80 },
	{ 35, 70 },
	{ 60, 55 },
	{ 68, 42 },
	{ 73, 35 },
	{ 75, 25 },
	{ 85, 15 },
};


struct curve *fan_curve_arr;
int fan_curve_arr_len = 0;
int fan_curve_arr_total = 0;

int mkconfdir(const char *path) 
{
	const char *home = getenv("HOME");
	char full_path[128];
	strcpy(full_path, home);
	strcat(full_path, path);
	int err = mkdir(full_path, S_IRWXU | S_IXOTH | S_IROTH | S_IXGRP | S_IRGRP);
	if (err != 0 && errno != EEXIST) {
		printf("mkconfdir: mkdir at path %s failed errno = %d\n", full_path, errno);
		return -1;
	} else if (errno == EEXIST) {
		return 1;
	} else {
		return 0;
	}
}

void init_fan_curve_conf(void)
{
	char path[128];
	int no_more_files = 0, i = 0;
	fan_curve_arr = malloc(sizeof(struct curve));
	fan_curve_arr_total = 1;
	strcpy(path, FAN_CURVE_CONFIG_PATH);
	while (no_more_files == 0) {
		char file_path[164];
		sprintf(file_path, "%s%d", path, i);
		if (fan_curve_arr_len + 1 > fan_curve_arr_total) {
			fan_curve_arr = realloc(fan_curve_arr, sizeof(struct curve) * (fan_curve_arr_total + 5));
			fan_curve_arr_total += 5;
			memset(&fan_curve_arr[fan_curve_arr_len], 0, sizeof(struct curve) * (fan_curve_arr_total - fan_curve_arr_len));
		}
		no_more_files = load_curve(&fan_curve_arr[i].curve, fan_curve_arr[i].name, MAX_TEXT_SIZE, &fan_curve_arr[i].used_points, &fan_curve_arr[i].total_points, file_path);
		if (no_more_files != -1) {
			fan_curve_arr_len++;
		}
		i++;
	}
}

void shutdown_controller(void)
{
	for (int i = 0; i < 4; i++) {
		save_port(&ports[i]);
	}
	for (int i = 0; i < fan_curve_arr_len; i++) {
		char save_to[MAX_TEXT_SIZE];
		sprintf(save_to, "%s%d", FAN_CURVE_CONFIG_PATH, i);
		save_curve(fan_curve_arr[i].curve, fan_curve_arr[i].name, fan_curve_arr[i].used_points, save_to);
		free(fan_curve_arr[i].curve);
	}
	free(fan_curve_arr);
}

int init_controller(void)
{
	int err = mkconfdir(CONFIG_PATH);
	if (err == -1) {
		printf("init_controller: mkconfdir failed at path %s\n", CONFIG_PATH);
		return -1;
	}
	err = mkconfdir(PORT_ONE_CONFIG_PATH);
	if (err == -1) {
		printf("init_controller: mkconfdir failed at path %s\n", PORT_ONE_CONFIG_PATH);
		return -1;
	}
	err = mkconfdir(PORT_TWO_CONFIG_PATH);
	if (err == -1) {
		printf("init_controller: mkconfdir failed at path %s\n", PORT_TWO_CONFIG_PATH);
		return -1;
	}
	err = mkconfdir(PORT_THREE_CONFIG_PATH);
	if (err == -1) {
		printf("init_controller: mkconfdir failed at path %s\n", PORT_THREE_CONFIG_PATH);
		return -1;
	}
	err = mkconfdir(PORT_FOUR_CONFIG_PATH);
	if (err == -1) {
		printf("init_controller: mkconfdir failed at path %s\n", PORT_FOUR_CONFIG_PATH);
		return -1;
	}
	init_fan_curve_conf();
	return 0;
}

void remove_curve(int index)
{
	if (fan_curve_arr_len <= 1) return;
	free(fan_curve_arr[index].curve);
	for (int i = index; i < fan_curve_arr_len-1; i++) {
		fan_curve_arr[i].curve = fan_curve_arr[i+1].curve;
		fan_curve_arr[i].total_points = fan_curve_arr[i+1].total_points;
		fan_curve_arr[i].used_points = fan_curve_arr[i+1].used_points;
	}
	fan_curve_arr_len -= 1;
}

void add_curve(void)
{
	if (fan_curve_arr_len + 1 > fan_curve_arr_total) {
		struct curve* tmp = realloc(fan_curve_arr, sizeof(struct curve) * (fan_curve_arr_len + 1));
		if (tmp == NULL) {
			printf("add_curve: failed to reallocate fan_curve_arr\n");
			return;
		}
		fan_curve_arr = tmp;
		fan_curve_arr_total += 1;
	}
	fan_curve_arr[fan_curve_arr_len].curve = alloc_point_arr(1);
	fan_curve_arr[fan_curve_arr_len].total_points = 1;
	fan_curve_arr[fan_curve_arr_len].used_points = 0;
	fan_curve_arr_len++;
}

int set_fan_count(struct port *p, int fc)
{
	char path[MAX_TEXT_SIZE];

	strcpy(path, p->proc_path);
	strcat(path, "/fan_count");
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		printf("set_fan_count: failed to open file at %s\n", path);
		return -1;
	}
	fprintf(f, "%d", fc);
	p->fan_count = fc;
	fclose(f);

	return 0;
}

int get_fan_count_from_driver(struct port *p)
{
	char path[MAX_TEXT_SIZE];

	strcpy(path, p->proc_path);
	strcat(path, "/fan_count");
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		printf("get_fan_count_from_driver: failed to open file at %s\n", path);
		return -1;
	}
	int fc = 0;
	fscanf(f, "%d", &fc);
	fclose(f);

	return fc;
}

int get_fan_speed_rpm(const char *p)
{
	char path[MAX_TEXT_SIZE];

	strcpy(path, p);
	strcat(path, "/fan_speed");
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		printf("get_fan_speed_rpm: failed to open file at %s\n", path);
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
		printf("get_fan_speed_pro: failed to open file at %s\n", path);
		return -1;
	}
	int speed_pro, speed_rpm;
	fscanf(f, "%d %d", &speed_pro, &speed_rpm);
	fclose(f);

	return speed_pro;
}

int load_port(struct port *p)
{
	char path[MAX_TEXT_SIZE];

	const char *home_path = getenv("HOME");

	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/fan_count"); 

	FILE *f = fopen(path, "r");
	p->fan_count = 4;
	if (f == NULL) {
		printf("load port failed to open file at path %s, ", path);
		if (errno == ENOENT) {
			printf("creating file now\n");
			f = fopen(path, "w");
			fclose(f);
		} else {
			printf("error = %d\n", errno);
			return -1;
		}
	} else {
		fscanf(f, "%d", &p->fan_count);
		fclose(f);
	}

	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/fan_curve"); 
	f = fopen(path, "r");
	if (f == NULL) {
		printf("load port: failed to open file at %s, ", path);
		if (errno == ENOENT) {
			printf("creating file now and setting curve to default\n");
			p->curve_i = 0;
			f = fopen(path, "w");
			if (f != NULL) {
				fprintf(f, "%d", 0);
				fclose(f);
			}
		} else {
			printf("errno = %d\n", errno);
			return -1;
		}
	} else {
		fscanf(f, "%d", &p->curve_i);
		fclose(f);
	}

	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/inner_rgb"); 

	int rgb_mode;
	f = fopen(path, "r");
	rgb_mode = 0, p->rgb.inner_speed = 0, p->rgb.inner_brightnes = 0, p->rgb.inner_direction = 0;
	p->rgb.inner_mode = &rgb_modes[rgb_mode];
	if (f == NULL) {
		printf("failed to open file at %s, ", path);
		if (errno == ENOENT) {
			printf("creating file now\n");
			f = fopen(path, "w");
			fclose(f);
		} else {
			printf("error = %d\n", errno);
			return -1;
		}
	} else {
		fscanf(f, "%d %d %d %d", &rgb_mode, &p->rgb.inner_speed, &p->rgb.inner_brightnes, &p->rgb.inner_direction);
		p->rgb.inner_mode = &rgb_modes[rgb_mode];
		fclose(f);
	}
	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/inner_colors");

	f = fopen(path, "r");
	for (int i = 0; i < 48; i++) {
		p->rgb.inner_color[i].r = 0xff;
		p->rgb.inner_color[i].g = 0x00;
		p->rgb.inner_color[i].b = 0x00;
	}
	if (f == NULL) {
		printf("load_port: failed to open file at path %s, ", path);
		if (errno == ENOENT) {
			printf("creating file now\n");
			f = fopen(path, "w");
			fclose(f);
		} else {
			printf("error = %d\n", errno);
			return -1;
		}
	} else {
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
	}
	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/outer_rgb"); 

	f = fopen(path, "r");
	rgb_mode = 0, p->rgb.outer_speed = 0, p->rgb.outer_brightnes = 0, p->rgb.outer_direction = 0;
	p->rgb.outer_mode = &rgb_modes[rgb_mode];
	if (f == NULL) {
		printf("load_port: failed to open file at path %s, ", path);
		if (errno == ENOENT) {
			printf("creating file now\n");
			f = fopen(path, "w");
			fclose(f);
		} else {
			printf("error = %d\n", errno);
			return -1;
		}
	} else {
		fprintf(f, "%d %d %d %d", rgb_mode, p->rgb.outer_speed, p->rgb.outer_brightnes, p->rgb.outer_direction);
		p->rgb.outer_mode = &rgb_modes[rgb_mode];
		fclose(f);
	}
	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/outer_colors");

	f = fopen(path, "r");
	for (int i = 0; i < 72; i++) {
		p->rgb.outer_color[i].r = 0xff;
		p->rgb.outer_color[i].g = 0;
		p->rgb.outer_color[i].b = 0;
	}
	if (f == NULL) {
		printf("load_port: failed to open file at path %s, ", path);
		if (errno == ENOENT) {
			printf("creating file now\n");
			f = fopen(path, "w");
			fclose(f);
		} else {
			printf("error = %d\n", errno);
			return -1;
		}
	} else {
		char line[351];
		fread(line, sizeof(char), 351, f);
		int stri = 0;
		for (int i = 0; i < 72; i++) {
			unsigned int tmpr, tmpg, tmpb;
			sscanf(&line[stri], "%02x%02x%02x", &tmpr, &tmpb, &tmpg);
			p->rgb.outer_color[i].r = tmpr;
			p->rgb.outer_color[i].g = tmpg;
			p->rgb.outer_color[i].b = tmpb;
			stri += 6;
		}
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

	strcpy(path, home_path);
	strcat(path, p->config_path); 
	strcat(path, "/fan_curve"); 
	f = fopen(path, "w");
	fprintf(f, "%d", p->curve_i);
	fclose(f);

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

struct point *alloc_point_arr(int size)
{
	struct point *p = calloc(size, sizeof(struct point));
	return p;
}

void resize_point_arr(struct point **arr, int *total_points, int new_size) 
{
	struct point *new_p = (struct point *)realloc(*arr, sizeof(struct point) * new_size);
	*total_points = new_size;
	*arr = new_p;
}

void reallocate_curve(struct point **curves_points, int *points_used, int *points_total, int additional_points) 
{
	struct point *new_curve = (struct point *)realloc(*curves_points, (*points_total + additional_points) * sizeof(struct point));
	if (new_curve == NULL) {
	    fprintf(stderr, "Failed to reallocate memory for points array\n");
	    return;
	}
	// Update the pointer and total_points
	*curves_points = new_curve;
	*points_total += additional_points;

}


int load_curve(struct point **p, char *name, int name_len, int *points_used, int *points_total, char *path)
{
	int ret = 0;
	const char *home_path = getenv("HOME");
	if (home_path == NULL) {
		printf("load_graph failed to get home_path\n");
		return -1;
	}
	if (*p == NULL || *points_total == 0) {
		*p = alloc_point_arr(7);
		*points_total = 7;
	}
	char new_path[100];
	strcpy(new_path, home_path);
	strcat(new_path, path);
	printf("load_curve:\np = %p, *p = %p, points_used = %d, points_total = %d\npath = %s, new_path = %s\n", (void *)p, (void *)*p, *points_used, *points_total, path, new_path);
	FILE *f = fopen(new_path, "r");
	if (f == NULL) {
		printf("load_curve: failed to open file at path %s, ", new_path);
		if (fan_curve_arr_len < 1 && errno == ENOENT) {
			printf("save_curve called\n");
			save_curve(default_fan_curve, "default", 7, path);
			f = fopen(new_path, "r");
			ret = 1;
		} else {
			printf("error = %d\n", errno);
			return -1;
		}
		
	} 
	char *line = malloc(sizeof(char)*100);
	size_t n = sizeof(char)*100;
	int fan_speed, ct;

	int i = 0;
	getline(&line, &n, f);
	printf("line = %s, name = %s, name_len = %d\n", line, name, name_len);
	if (name != NULL && line[0] != '\n') strncpy(name, line, name_len);
	else if (line[0] == '\n') {
		name[0] = '0';
		name[1] = '\0';
	}
	printf("name after = %s\n", name);
	while (getline(&line, &n, f) != -1) {
		sscanf(line, "%d %d", &fan_speed, &ct);
		if (*points_used + 1 > *points_total) {
			reallocate_curve(p, points_used, points_total, 5);
		}
		(*p)[i].x = ct, (*p)[i].y = fan_speed;
		*points_used = ++i;

	}
	free(line);
	fclose(f);
	return ret;
}

int save_curve(struct point *p, char *name, int points_used, char *path)
{
	if (p == NULL) {
		printf("save_fan_curve: error p == NULL\n");
		return -1;
	}
	const char *home_path = getenv("HOME");
	if (home_path == NULL) {
		printf("save_fan_curve: failed to get home_path\n");
		return -1;
	}
	char new_path[100];
	strcpy(new_path, home_path);
	strcat(new_path, path);
	FILE *f = fopen(new_path, "w");
	if (f == NULL) {
		printf("save_fan_curve: failed to open file at path %s, errno = %d\n", new_path, errno);
		return -1;
	}
	if (name != NULL) fprintf(f, "%s", name);
	else fputc('\n', f);
	for (int i = 0; i < points_used; i++) {
		fprintf(f, "%d %d\n", p[i].y, p[i].x);
	}
	fclose(f);
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

int set_fan_curve(struct port *p)
{
	if (p == NULL) {
		printf("set_fan_curve: error p == NULL\n");
		return -1;
	}
	char new_path[100];
	strcpy(new_path, p->proc_path);
	strcat(new_path, "/fan_curve");
	FILE *f = fopen(new_path, "w");
	if (f == NULL) {
		printf("set_fan_curve: failed to open file at path %s\n", new_path);
		return -1;
	}
	printf("set_fan_curve: open file at path %s\n", new_path);

	char tmp_str[100];
	int str_i = 0;
	for (int i = 0; i < fan_curve_arr[p->curve_i].used_points; i++) {
		sprintf(&tmp_str[str_i], "%03d %03d\n", fan_curve_arr[p->curve_i].curve[i].y, fan_curve_arr[p->curve_i].curve[i].x);
		str_i += 8;
	}
	fputs(tmp_str, f);
	fclose(f);
	return 0;
}

float get_fan_speed_from_graph(struct graph *g, float temp) 
{
	for (int i = 0; i < g->point_amount; i++) {
		float xone = (float)g->points[i].x, xtwo = (float)g->points[i + 1].x;
		float yone = 100.0 - g->points[i].y, ytwo = 100.0 - g->points[i + 1].y;
		if (xone <= temp && xtwo >= temp) {
			float a = (ytwo - yone)/(xtwo - xone);
			float b = yone - a * xone;
			return a * temp + b;
		}
	}
	return 0.0;
}

int set_fan_speed(struct port *p, int speed)
{
	char path[MAX_TEXT_SIZE];

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
	char path[MAX_TEXT_SIZE];
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
	char path[MAX_TEXT_SIZE];
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

	char path[MAX_TEXT_SIZE];
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

	char path[MAX_TEXT_SIZE];
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
	char outer_color_str[fan_count * 72];
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
	char inner_color_str[fan_count * 48];
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





