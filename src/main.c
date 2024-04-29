#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>

#include "../include/ui.h"


//Bus 001 Device 006: ID 0cf2:a104 ENE Technology, Inc. LianLi-UNI FAN-AL V2-v0.4

/*
void updatetemp(text *cput, SDL_Rect *pos) {
	FILE *fcpu = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	int icputemp;
	fscanf(fcpu, "%d", &icputemp);
	float cputemp = icputemp/1000.0;
	char str[10];
	sprintf(str, "%.2f", cputemp); 
	changeText(cput, str, pos, &white, font);
	fclose(fcpu);
}
*/

void buttonclick()
{
	printf("hello\n");
}

SDL_Color grey = { 209, 209, 209, SDL_ALPHA_OPAQUE };
SDL_Color darkgrey = { 84, 84, 84, SDL_ALPHA_OPAQUE };

int main()
{
	strcpy(font_path, "/usr/share/fonts/TTF/HackNerdFont-Regular.ttf");
	if (ui_init()) {
		printf("ui_init failed\n");
		ui_shutdown();
		return 1;
	}

	running = 1;
	SDL_Event event;
	unsigned int a = SDL_GetTicks();
	unsigned int b = SDL_GetTicks();
	unsigned int b2 = SDL_GetTicks();
	double delta = 0;
	double delta2 = 0;
	
	int speeds_pro[4] = { 0 };
	int speeds_rpm[4] = { 0 };

	FILE *fan_speeds = fopen("/proc/fan_speeds", "r+");
	if (fan_speeds == NULL) {
		printf("file not found:(\n");
		ui_shutdown();
		return 1;
	}
	fscanf(fan_speeds, "port\t:\t1,\t2,\t3,\t4\n%%\t:\t%d,\t%d,\t%d,\t%d\nrpm\t:\t%d,\t%d,\t%d,\t%d\n", 
					&speeds_pro[0], &speeds_pro[1], &speeds_pro[2], &speeds_pro[3],
					&speeds_rpm[0], &speeds_rpm[1], &speeds_rpm[2], &speeds_rpm[3]);
	fclose(fan_speeds);

	char speeds_str[100];
	sprintf(speeds_str, "%d, %d, %d, %d", speeds_pro[0], speeds_pro[1], speeds_pro[2], speeds_pro[3]);
	struct text speeds = create_text(speeds_str, 10, 10, WHITE, BLACK, font);

	struct text intext = create_text("hello", 200, 300, WHITE, BLACK, font);
	struct input *inbo = create_input_from_text(intext, 1, font, GREEN, BLUE, WHITE);
	struct input *intwo = create_input("hello2", 0, 100, 100, 0, 0, font, grey, darkgrey, WHITE);
	struct button *button = create_button("print hello", 300, 10, 0, 0, font, buttonclick, WHITE, darkgrey, WHITE);

	while (running) {
		a = SDL_GetTicks();
		delta = a - b;
		delta2 = a - b2;

		if (SDL_PollEvent(&event)) {
			handle_event(&event);
		}

		if (delta > 1000/60.0) {
			b = a;
			clear_screen();
			render_text(&speeds, NULL);
			render_input_box(inbo);
			render_input_box(intwo);
			render_button(button);
			show_screen();
		}
		if (delta2 > 1000/1.0) {
			b2 = a;
			fan_speeds = fopen("/proc/fan_speeds", "r+");
			if (fan_speeds == NULL) {
				printf("fan_speeds failed to open\n");
			} else {
				fscanf(fan_speeds, "port\t:\t1,\t2,\t3,\t4\n%%\t:\t%d,\t%d,\t%d,\t%d\nrpm\t:\t%d,\t%d,\t%d,\t%d\n", 
						&speeds_pro[0], &speeds_pro[1], &speeds_pro[2], &speeds_pro[3],
						&speeds_rpm[0], &speeds_rpm[1], &speeds_rpm[2], &speeds_rpm[3]);
				sprintf(speeds_str, "%d, %d, %d, %d", speeds_pro[0], speeds_pro[1], speeds_pro[2], speeds_pro[3]);
				change_text_and_render_texture(&speeds, speeds_str, speeds.fg_color, speeds.bg_color, font);
				fclose(fan_speeds);
			}
		}
		SDL_Delay(1);
	}
	destroy_text_texture(&speeds);
	destroy_text_texture(&intext);
	ui_shutdown();

	return 0;
}
