#include <stdio.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

//Bus 001 Device 006: ID 0cf2:a104 ENE Technology, Inc. LianLi-UNI FAN-AL V2-v0.4
#define VENDOR_ID 0x0cf2
#define PRODUCT_ID 0xa104

libusb_device_handle *hub;

//returns 0 if it is the fan hub
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

int init()
{

	int err = libusb_init(NULL);
	if (err) {
		printf("libusb_init failed err: %s\n", libusb_error_name(err));
		return 1;
	}
	libusb_device **list;
	libusb_device *found = NULL;
	ssize_t cnt = libusb_get_device_list(NULL, &list);
	ssize_t i = 0;
	if (cnt > 0) {
		for (i = 0; i < cnt; i++) {
			libusb_device *device = list[i];
			if (isFanHub(device)) {
				found = device;
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

int shutdown()
{
	libusb_release_interface(hub, 0x01);
	libusb_close(hub);
	libusb_exit(NULL);
	return 0;
}
int setOuterColor(int r, int g, int b, int port, int fanCount)
{
	int err = 0;
	int datalen = 353;
	unsigned char data[4][353] = { { 0xe0, 0x10, 0x60, port, fanCount }, { 0xe0, 0x2f + (port * 2) }, 
								   { 0xe0, 0x0e + (port * 2), 0x01, 0x02 }, { 0xe0, 0x0f + (port * 2), 0x01, 0x02 } };
	for (int i = 2; i < 54 * fanCount; i+=3) {
		data[1][i]     = r;
		data[1][i + 1] = b;
		data[1][i + 2] = g;
	}
	for (int i = 0; i < 4; i++) {
		err = libusb_control_transfer(hub, 0x21, 0x09, 0x02e0, 1, data[i], 353, 1000);
		printf("send %d\n", i);
		if (err != datalen) printf("sending %d failed err: %s\n", i, libusb_error_name(err));
	}
	return 0;


}
int setInnerColor(int r, int g, int b, int port, int fanCount)
{
	int err = 0;
	int datalen = 353;
	unsigned char data[4][353] = { { 0xe0, 0x10, 0x60, port, fanCount }, { 0xe0, 0x2e + (port * 2) },
								   { 0xe0, 0x0e + (port * 2), 0x01, 0x02 }, { 0xe0, 0x0f + (port * 2), 0x01, 0x02 } };
	for (int i = 2; i < 24 * fanCount; i += 3) {
		data[1][i]     = r;
		data[1][i + 1] = b;
		data[1][i + 2] = g;
	}
	for (int i = 0; i < 3; i++) {
		err = libusb_control_transfer(hub, 0x21, 0x09, 0x02e0, 1, data[i], datalen, 1000);
		if (err != datalen) printf("sending %d failed err: %s\n", i, libusb_error_name(err));
	}
	//TODO: put in a for loop and make a struct for data.
	return 0;

}
typedef struct fans {
	int curentSpeed[4];
} fans;
int setSpeed(unsigned int speed, int port)
{
	fans Fans;
	Fans.curentSpeed[port] = (speed > 100) ? 100 : speed;
	int err = 0;
	int datalen = 353;

	unsigned char data[5][353] = { { 0xe0, 0x50 }, { 0xe0, 0x20, 0x00, Fans.curentSpeed[0]}, 
								{ 0xe0, 0x21, 0x00, Fans.curentSpeed[1] }, { 0xe0, 0x22, 0x00, Fans.curentSpeed[2] }, { 0xe0, 0x23, 0x00, Fans.curentSpeed[3] } };

	for (int i = 0; i < 5; i++) {
		err = libusb_control_transfer(hub, 0x21, 0x09, 0x02e0, 1, data[i], datalen, 1000);
		if (err != datalen) printf("sending %d failed err: %s\n", i, libusb_error_name(err));
	}
	return 0;
}

int main()
{
	int err = init();
	if (err != 0) {
		printf("init failed %s\n", libusb_error_name(err));
		shutdown();
		return 1;
	}

	unsigned char data[4][353] = { { 0xe0, 0x10, 0x60, 0x01, 0x04 }, { 0xe0, 0x30 }, { 0xe0, 0x31 }, { 0xe0, 0x11, 0x04, 0xff } };

	libusb_control_transfer(hub, 0x21, 0x09, 0x02e0, 1, data[0], 353, 1000);

	shutdown();
	return 0;
}
