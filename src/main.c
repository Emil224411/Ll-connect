#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
	double delta = 0;

	int speeds_pro[4] = { 0 };
	int speeds_rpm[4] = { 0 };

	FILE *fan_speeds = fopen("/proc/fan_speeds", "r+");
	fscanf(fan_speeds, "port\t:\t1,\t2,\t3,\t4\n%%\t:\t%d,\t%d,\t%d,\t%d\nrpm\t:\t%d,\t%d,\t%d,\t%d\n", 
					&speeds_pro[0], &speeds_pro[1], &speeds_pro[2], &speeds_pro[3],
					&speeds_rpm[0], &speeds_rpm[1], &speeds_rpm[2], &speeds_rpm[3]);

	char speeds_str[100];
	sprintf(speeds_str, "%d, %d, %d, %d", speeds_pro[0], speeds_pro[1], speeds_pro[2], speeds_pro[3]);
	struct text test = create_text(speeds_str, 100, 100, &white, font);

	while (running) {
		a = SDL_GetTicks();
		delta = a - b;

		if (SDL_PollEvent(&event)) {
			handle_event(&event);
		}

		if (delta > 1000/60.0) {
			b = a;
			clear_screen();
			render_text(&test);
			SDL_RenderPresent(renderer);
		}
		SDL_Delay(1);
	}
	destroy_text_texture(&test);
	ui_shutdown();

	return 0;
}
