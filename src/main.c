#include <stdio.h>
#include <stdlib.h>
#include "../include/controller.h"
#include "../include/ui.h"

//Bus 001 Device 006: ID 0cf2:a104 ENE Technology, Inc. LianLi-UNI FAN-AL V2-v0.4

page mainMenu;
page configMenu;

void mainMenuButton()
{
	currentPage = &mainMenu;
}

void configMenuButton()
{
	currentPage = &configMenu;
}

void updatetemp(text *cput, SDL_Rect *pos) {
	

}

int main()
{
	if (ui_init()) {
		printf("ui_init failed\n");
		ui_shutdown();
		return 1;
	}
	if (initController()) {
		printf("init failed\n");
		shutdownController();
		ui_shutdown();
		return 1;
	}

	mainMenu.pageColor = black;
	configMenu.pageColor = black;

	SDL_Rect mpos = { 10, 20 };
	SDL_Rect mcpos = { 10, 20 };
	SDL_Rect cpos = { 10, 50 };
	SDL_Rect cmpos = { 10, 50 };
	SDL_Rect mcputemppos = { 150, 20 };
	text *mmenutxt = createText("Main Menu", &mpos, &white, &mainMenu, font);
	text *mmenutxtc = createText("Main Menu", &mcpos, &white, &configMenu, font);
	text *cmenutxtm = createText("Config Menu", &cmpos, &white, &mainMenu, font);
	text *cmenutxt = createText("Config Menu", &cpos, &white, &configMenu, font);
	text *mcputemptxt = createText("00.00", &mcputemppos, &white, &mainMenu, font);
	
	createButton(mpos, mmenutxt, &green, mainMenuButton, &mainMenu);
	createButton(mcpos, mmenutxtc, &blue, mainMenuButton, &configMenu);
	createButton(cmpos, cmenutxtm, &blue, configMenuButton, &mainMenu);
	createButton(cpos, cmenutxt, &green, configMenuButton, &configMenu);
	currentPage = &configMenu;
	
	running = 1;
	SDL_Event event;
	unsigned int a = SDL_GetTicks();
	unsigned int b = SDL_GetTicks();
	unsigned int b2 = SDL_GetTicks();
	double delta = 0;
	double delta2 = 0;

	while (running) {
		a = SDL_GetTicks();
		delta = a - b;
		delta2 = a - b2;

		if (SDL_PollEvent(&event)) {
			hadleEvent(&event);
		}
		if (delta2 > 2000) {
			b2 = a;
			FILE *fcpu = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
			int icputemp;
			fscanf(fcpu, "%d", &icputemp);
			float cputemp = icputemp/1000.0;
			char str[10];
			sprintf(str, "%.2f", cputemp); 
			changeText(mcputemptxt, str, &mcputemppos, &white, font);
			fclose(fcpu);
		}
		if (delta > 1000/60.0) {
			b = a;
			clearscreen();
			
			renderPage(currentPage);

			SDL_RenderPresent(renderer);
		}
	}
	//mainloop(mcputemptxt, &mcputemppos);
	shutdownController();
	destroyPage(&mainMenu);
	destroyPage(&configMenu);
	ui_shutdown();

	return 0;
}
