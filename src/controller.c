#include "../include/controller.h"

int isFanHub(libusb_device *dev)
{
	struct libusb_device_descriptor dsc = {0};
	int err = 0;

	err = libusb_get_device_descriptor(dev, &dsc);
	if (err) {
		printf("libusb_get_device_descriptor failed err: %s\n", libusb_error_name(err));
		return 0;
	}
	if (dsc.idVendor == VENDOR_ID && dsc.idProduct == PRODUCT_ID) return 1;
	else return 0;

}

int initController()
{
	int err = 0;
	ssize_t count;
	libusb_device **list;
	libusb_device *found = NULL;

	err = libusb_init(NULL);
	if (err) {
		printf("libusb_init failed err: %s\n", libusb_error_name(err));
		return 1;
	}
	
	count = libusb_get_device_list(NULL, &list);
	if (count > 0) {
		for (ssize_t i = 0; i < count; i++) {
			if (isFanHub(list[i])) {
				found = list[i];
				break;
			}
		}

		if (found) {
			err = libusb_open(found, &hub);
			if (err) printf("libusb_open failed err: %s\n", libusb_error_name(err));
			else {
				if (libusb_kernel_driver_active(hub, 0x01)) {
					err = libusb_detach_kernel_driver(hub, 0x01);
					if (err) printf("libusb_detach_kernel_driver failed err: %s\n", libusb_error_name(err));
				}
				err = libusb_claim_interface(hub, 0x01);
				if (err) printf("libusb_claim_interface failed err: %s\n", libusb_error_name(err));
			}
		}
	}
	libusb_free_device_list(list, 1);
	return err;
}

void shutdownController()
{
	if (hub != NULL) {
		libusb_release_interface(hub, 0x01);
		libusb_close(hub);
	}
	libusb_exit(NULL);
}

int setOuterColor(int r, int g, int b, int port, int fanCount)
{
	int datalen = 353;
	unsigned char data[4][353] = { { 0xe0, 0x10, 0x60, port, fanCount }, { 0xe0, 0x2f + (port * 2) }, 
								   { 0xe0, 0x0e + (port * 2), 0x01, 0x02 }, { 0xe0, 0x0f + (port * 2), 0x01, 0x02 } };
	for (int i = 2; i < 54 * fanCount; i+=3) {
		data[1][i]     = r;
		data[1][i + 1] = b;
		data[1][i + 2] = g;
	}
	for (int i = 0; i < 4; i++) {
		int ret = libusb_control_transfer(hub, 0x21, 0x09, 0x02e0, 1, data[i], datalen, 1000);
		if (ret != datalen) printf("sending %d failed err: %s\n", i, libusb_error_name(ret));
	}
	return 0;
}

//buffer should be 65 in size
void getCurrentSpeed(unsigned char *buffer)
{
	int ret = libusb_control_transfer(hub, 0xa1, 0x01, 0x01e0, 0x80, 0, 0, 1000);
	unsigned char response[65];
	ret = libusb_control_transfer(hub, 0xa1, 0x01, 0x01e0, 1, response, sizeof(response), 1000);
}

int setInnerColor(int r, int g, int b, int port, int fanCount)
{
	int datalen = 353;
	unsigned char data[4][353] = { { 0xe0, 0x10, 0x60, port, fanCount }, { 0xe0, 0x2e + (port * 2) },
								   { 0xe0, 0x0e + (port * 2), 0x01, 0x02 }, { 0xe0, 0x0f + (port * 2), 0x01, 0x02 } };
	for (int i = 2; i < 24 * fanCount; i += 3) {
		data[1][i]     = r;
		data[1][i + 1] = b;
		data[1][i + 2] = g;
	}
	for (int i = 0; i < 3; i++) {
		int ret = libusb_control_transfer(hub, 0x21, 0x09, 0x02e0, 1, data[i], datalen, 1000);
		if (ret != datalen) printf("sending %d failed err: %s\n", i, libusb_error_name(ret));
	}
	return 0;

}

int setSpeed(unsigned int speed, int port)
{
	speed = (speed < 0x0c) ? 0x0c : (speed > 0x64) ? 0x64 : speed;
	unsigned char data[2][353] = { { 0xe0, 0x50 }, { 0xe0, 0x1f+port, 0x00, speed} };

	int ret = libusb_control_transfer(hub, 0x21, 0x09, 0x02e0, 1, data[0], sizeof(data[0]), 1000);
	if (ret != sizeof(data[0])) printf("setSpeed sending header failed err: %s\n", libusb_error_name(ret));
	ret     = libusb_control_transfer(hub, 0x21, 0x09, 0x02e0, 1, data[1], sizeof(data[1]), 1000);
	if (ret != sizeof(data[1])) printf("setSpeed sending speed failed err: %s\n", libusb_error_name(ret));

	return 0;
}
