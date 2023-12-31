#include <stdio.h>
#include "../include/controller.h"
#include "../include/ui.h"

//Bus 001 Device 006: ID 0cf2:a104 ENE Technology, Inc. LianLi-UNI FAN-AL V2-v0.4

void buttonred()
{
	setOuterColor(0xff, 0x00, 0x00, 0x01, 0x04);
	setInnerColor(0xff, 0x00, 0x00, 0x01, 0x04);
}

void buttongreen()
{
	setOuterColor(0x00, 0xff, 0x00, 0x01, 0x04);
	setInnerColor(0x00, 0xff, 0x00, 0x01, 0x04);
}

void buttonblue()
{
	setOuterColor(0x00, 0x00, 0xff, 0x01, 0x04);
	setInnerColor(0x00, 0x00, 0xff, 0x01, 0x04);
}

int main()
{
	ui_init();
	buttonarr[0].funtion = &buttonred;
	buttonarr[0].pos.h = 50;
	buttonarr[0].pos.w = 100;
	buttonarr[0].pos.x = 200;
	buttonarr[0].pos.y = 100;
	buttonarr[0].color = &red;
	buttonarr[1].funtion = &buttongreen;
	buttonarr[1].pos.h = 50;
	buttonarr[1].pos.w = 100;
	buttonarr[1].pos.x = 350;
	buttonarr[1].pos.y = 100;
	buttonarr[1].color = &green;
	buttonarr[2].funtion = &buttonblue;
	buttonarr[2].pos.h = 50;
	buttonarr[2].pos.w = 100;
	buttonarr[2].pos.x = 500;
	buttonarr[2].pos.y = 100;
	buttonarr[2].color = &blue;

	int err = initController();
	if (err != 0) {
		printf("init failed\n");
		shutdownController();
		return 1;
	}
	mainloop();
	shutdownController();

	return 0;
}
