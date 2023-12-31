#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <libusb-1.0/libusb.h>
#include <stdio.h>

#define VENDOR_ID 0x0cf2
#define PRODUCT_ID 0xa104

typedef struct fans {
	int curentSpeed[4];
} fans;

libusb_device_handle *hub;

int isFanHub(libusb_device *dev);
int initController();
int shutdownController();
int setOuterColor(int r, int g, int b, int port, int fanCount);
int setInnerColor(int r, int g, int b, int port, int fanCount);
int setSpeed(unsigned int speed, int port);

#endif
