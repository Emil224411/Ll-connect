#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/usb.h>

#define VENDOR_ID 0x0cf2
#define PRODUCT_ID 0xa104

MODULE_LICENSE("GPL");

static struct usb_device *dev;
static struct kobject *lian_li_hub_kobj;


static struct usb_device_id dev_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{},
};
MODULE_DEVICE_TABLE(usb, dev_table);

struct rgb_data {
	int mode, brightness, speed, direction;
};

struct port {
	int fan_speed, fan_speed_rpm, fan_count;
	struct rgb_data rgb;
};
static struct port ports[4];

/*
 * port from 1 to 4
 * sets inner and outer colors with individual packets
 * 1. header 0x10, 0x60
 * 2. inner  0x30 
 * 3. outer  0x31
 * 4. outer  0x11 
 * 5. inner  0x10 
 */ 
static void set_rgb(int port, int fan_count, unsigned char *inner_colors, unsigned char *outer_colors, int mode, int speed, int brightness, int direction)
{
	ports[port-1].rgb.mode 	     = mode;
	ports[port-1].rgb.brightness = brightness;
	ports[port-1].rgb.speed      = speed;
	ports[port-1].rgb.direction  = direction;
	int ret;
	const size_t packet_size = 353;
	unsigned char *buffer = kcalloc(packet_size, sizeof(*buffer), GFP_KERNEL);
	unsigned char data[5][353] = { { 0xe0, 0x10, 0x60, port, fan_count }, 
					{ 0xe0, 0x30 + (2 * (port - 1)), },  
					{ 0xe0, 0x31 + (2 * (port - 1)), },  
					{ 0xe0, 0x11 + (2 * (port - 1)), mode, speed, direction, brightness },  
					{ 0xe0, 0x10 + (2 * (port - 1)), mode, speed, direction, brightness },  
	};
	memcpy(&data[1][2], inner_colors, packet_size - 2);
	memcpy(&data[2][2], outer_colors, packet_size - 2);
	for (int i = 0; i < 5; i++) {
		memcpy(buffer, data[i], packet_size);

		ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0x0), 0x09, 0x21, 0x02e0, 0x01, buffer, packet_size, 1000);
		if (ret != packet_size) {
			printk("Lian Li Hub: set_rgb failed sending packet %d error %d", i, ret);
		}
	}
}

//buffer should be 65 in size
static void get_speed(void)
{
	int size = 65;
	unsigned char *response = kcalloc(size, sizeof(*response), GFP_KERNEL);
	int ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0x80), 0x01, 0xa1, 0x01e0, 1, response, size, 1000);
	if (ret != size) {
		printk("Lian Li Hub: get_speed failed error %d\n", ret);
	} else {
		ports[0].fan_speed_rpm = (response[2] << 8) + response[3];
		ports[1].fan_speed_rpm = (response[4] << 8) + response[5];
		ports[2].fan_speed_rpm = (response[6] << 8) + response[7];
		ports[3].fan_speed_rpm = (response[8] << 8) + response[9];
	}
	kfree(response);
}

static void set_speed(int new_speeds[])
{
	int datalen = 353;
	unsigned char *mbuff = kcalloc(datalen, sizeof(*mbuff), GFP_KERNEL);
	unsigned char data[5][353] = { { 0xe0, 0x50, }, { 0xe0, 0x20, 0x00, new_speeds[0], }, 
					{ 0xe0, 0x21, 0x00, new_speeds[1], }, { 0xe0, 0x22, 0x00, new_speeds[2], }, { 0xe0, 0x23, 0x00, new_speeds[3], } };
	for (int i = 0; i < 5; i++) {
		memcpy(mbuff, data[i], 353);
		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, mbuff, datalen, 100);
		if (ret != datalen) {
			printk("Lian Li Hub - failed to send usb_control_msg with data[%d] err: %d", i, ret);
		} else if (i >= 1) {
			ports[i-1].fan_speed = new_speeds[i-1];
		}
	}

	kfree(mbuff);
	printk("Lian Li Hub - speed set to %d %d %d %d", ports[0].fan_speed, ports[1].fan_speed, ports[2].fan_speed, ports[3].fan_speed);
}

static ssize_t show_fan_speeds(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	get_speed();
	size_t textsize = 128;

	
	int ret = sysfs_emit(buf, "port\t:\t1,\t2,\t3,\t4\n%%\t:\t%d,\t%d,\t%d,\t%d\nrpm\t:\t%d,\t%d,\t%d,\t%d\n", 
			ports[0].fan_speed, ports[1].fan_speed, ports[2].fan_speed, ports[3].fan_speed,
			ports[0].fan_speed_rpm, ports[1].fan_speed_rpm, ports[2].fan_speed_rpm, ports[3].fan_speed_rpm);
	return ret;
}

static ssize_t store_fan_speeds(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int new_speed[4];


	sscanf(buf, "%d %d %d %d", &new_speed[0], &new_speed[1], &new_speed[2], &new_speed[3]);

	set_speed(new_speed);

	return min(count, 32);
}

static struct kobj_attribute fan_speed_attr = __ATTR(fan_speed, 0660, show_fan_speeds, store_fan_speeds);

static int dev_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	printk("Lian Li Hub - driver probe\n");

	dev = interface_to_usbdev(intf);
	if (dev == NULL) {
		printk("Lian Li Hub - error getting dev from intf");
		return -1;
	}
	lian_li_hub_kobj = kobject_create_and_add("Lian_li_hub", kernel_kobj);
	if (lian_li_hub_kobj == NULL) {
		printk("Lian Li Hub: error creating and adding kobj\n");
		return -1;
	}
	if (sysfs_create_file(lian_li_hub_kobj, &fan_speed_attr.attr)) {
		printk("Lian Li Hub: error creating sysfs file fan speed");
		kobject_put(lian_li_hub_kobj);
		return -1;
	}
	int defaultspeeds[] = { 30, 30, 40, 0 };
	set_speed(defaultspeeds);

	get_speed();

	unsigned char inner[351] = { 0 };
	unsigned char outer[351] = { 0 };
	
	for (int i = 0; i < 96; i += 3) {
		inner[i]     = 0xff;
		inner[i + 1] = 0x00;
		inner[i + 2] = 0x00;
	}
	for (int i = 0; i < 144; i += 3) {
		outer[i]     = 0xff;
		outer[i + 1] = 0xff;
		outer[i + 2] = 0xff;
	}
	
	set_rgb(1, 4, inner, outer, 1, 2, 0, 0);

	return 0;
}

static void dev_disconnect(struct usb_interface *intf)
{
	printk("Lian Li Hub - driver disconnect\n");
	kobject_put(lian_li_hub_kobj);
}

static struct usb_driver driver = {
	.name = "lianli AL120",
	.id_table = dev_table,
	.disconnect = dev_disconnect,
	.probe = dev_probe,
};

static int __init controller_init(void)
{
	int res;
	printk("Lian Li Hub - driver init\n");
	res = usb_register(&driver);
	if (res) {
		printk("Lian Li Hub: Error during usb register\n");
		return -res;
	}
	
	return 0;
}

static void __exit controller_exit(void)
{
	usb_deregister(&driver);
	printk("Lian Li Hub - driver exit\n");
}

module_init(controller_init);
module_exit(controller_exit);
