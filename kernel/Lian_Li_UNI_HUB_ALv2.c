#include <linux/init.h>
#include <linux/hid.h>
#include <linux/hwmon.h>
#include <linux/module.h>
#include <linux/string.h>

#define VENDOR_ID  0x0cf2
#define PRODUCT_ID 0xa104

#define PACKET_SIZE 353

#define MSG_START 0xe0

#define SET_SPEED 0x5000

#define PORT_AMOUNT 4
#define PORT_ONE   0x20
#define PORT_TWO   0x21
#define PORT_THREE 0x22
#define PORT_FOUR  0x23

struct header {
	u8 magic;
	u16 type;
};

struct drv_data {
	struct hid_device *dev;
	struct device *hwmon;

	u16 rpm[PORT_AMOUNT];
	u8 speed[PORT_AMOUNT];

	u8 buffer[PACKET_SIZE];
};


static int mb_sync_state;

static void get_speed(void);
static int set_speed(struct drv_data *drv, int chanel, long val);

static void mb_sync(int enable);


static void get_speed(void) {
	int size = 65;
	// usb_control_msg(dev, usb_rcvctrlpipe(dev, 0x80), 0x01, 0xa1, 0x01e0, 1, response, size, 1000);
	// ports[0].fan_speed_rpm = (response[2] << 8) + response[3];
	// ports[1].fan_speed_rpm = (response[4] << 8) + response[5];
	// ports[2].fan_speed_rpm = (response[6] << 8) + response[7];
	// ports[3].fan_speed_rpm = (response[8] << 8) + response[9];
}

static void set_speeds(int port_one, int port_two, int port_three, int port_four) {
	int new_speeds[] = { port_one, port_two, port_three, port_four };
	u8 data[5][PACKET_SIZE] =  { { 0xe0, 0x50, }, 
					{ 0xe0, 0x20, 0x00, new_speeds[0], }, 
					{ 0xe0, 0x21, 0x00, new_speeds[1], }, 
					{ 0xe0, 0x22, 0x00, new_speeds[2], }, 
					{ 0xe0, 0x23, 0x00, new_speeds[3], } };
	// for (int i = 0; i < 5; i++) {
	// 	memcpy(mbuff, data[i], PACKET_SIZE);
	// 	usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, mbuff, PACKET_SIZE, 100);
	// 	ports[i-1].fan_speed = new_speeds[i-1];
	// }

}

static int send_report(struct drv_data *drv, const void *data, size_t data_size) 
{
	size_t buff_size = sizeof(drv->buffer);

	if (data_size > buff_size) 
		return -EINVAL;

	memcpy(drv->buffer, data, data_size);

	if (data_size < buff_size)
		memset(drv->buffer + data_size, 0, buff_size - data_size);

	return hid_hw_output_report(drv->dev, drv->buffer, buff_size) < 0 ?: 0;
}

// old:
//	   usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, buffer, PACKET_SIZE, 100);
//	   usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, buffer, PACKET_SIZE, 100);
static int set_speed(struct drv_data *drv, int chanel, long val)
{
	/* 
	 * TODO:
	 *    is val 0-255 or is that unknown?
	 *    assuming chanel goes from 1-4, the port number the hub expects is 0x20-0x23
	 */
	u8 port = chanel+0x1f;
	u8 speed = val > 255 ? 255 : val < 0 ? 0 : val;

	struct header header = { MSG_START, SET_SPEED };
	/* the third item of the body is just filler(not used when setting speed) */
	u8 body[] = { MSG_START, port, 0x00, speed }; 

	int ret;
	ret = send_report(drv, &header, sizeof(header));
	/* should maybe do something here but idk if there is much to be done, 
	 * since its the hw/hid_hw_output_report that has a issue 
	 */
	if (ret) goto out; 
	ret = send_report(drv, &body, sizeof(body));
	if (ret) goto out;

	drv->speed[chanel-1] = speed;
out:
	return ret;
}

static void mb_sync(int enable)
{
	u8 *buffer;
	buffer[0] = 0xe0; 
	buffer[1] = 0x10; 
	buffer[2] = 0x61; 
	buffer[3] = enable;

	//usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, buffer, PACKET_SIZE, 100);

	mb_sync_state = enable;
}

static int uni_probe(struct hid_device *hid_dev, const struct hid_device_id *hid_dev_id)
{
	return 0;
}

static void uni_remove(struct hid_device *hid_dev)
{
	return;
}
static int uni_raw_event(struct hid_device *hid_dev, struct hid_report *rid, u8 *u, int i)
{
	return 0;
}

static const struct hid_device_id uni_hub_alv2_hid_id_table[] = {
	{ HID_USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{},
};

static struct hid_driver uni_hub_alv2_hid_driver = {
	.name = "uni-hub-ALv2",
	.id_table = uni_hub_alv2_hid_id_table,
	.probe = uni_probe,
	.remove = uni_remove,
	.raw_event = uni_raw_event,
};

static int __init init(void)
{
	return hid_register_driver(&uni_hub_alv2_hid_driver);
}

static void __exit exit(void)
{
	hid_unregister_driver(&uni_hub_alv2_hid_driver);
}
module_init(init);
module_exit(exit);

MODULE_AUTHOR("Emil <emil@svansoe.io>");
MODULE_DESCRIPTION("driver for UNI HUB ALv2");
MODULE_LICENSE("GPL");
