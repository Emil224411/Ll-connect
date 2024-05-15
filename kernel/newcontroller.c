#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/usb.h>

#define VENDOR_ID 0x0cf2
#define PRODUCT_ID 0xa104

MODULE_LICENSE("GPL");

struct rgb_data {
	int mode, brightnes, speed, direction;
};

struct port {
	int fan_speed, fan_speed_rpm, fan_count;
	struct rgb_data rgb;
};
static struct port ports[4];

static struct usb_device *dev;
static struct proc_dir_entry *proc_fan_speed;
static struct proc_dir_entry *proc_rgb;
static struct proc_dir_entry *proc_dir;

static struct usb_device_id dev_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{},
};
MODULE_DEVICE_TABLE(usb, dev_table);

void set_outer_color(int r, int g, int b, int port, int fan_cnt)
{
	int datalen = 353;
	unsigned char *mbuff = kcalloc(datalen, sizeof(unsigned char), GFP_KERNEL);
	unsigned char data[4][353] = { { 0xe0, 0x10, 0x60, port, fan_cnt }, { 0xe0, 0x2f + (port * 2) }, 
								   { 0xe0, 0x0e + (port * 2), 0x01, 0x02 }, { 0xe0, 0x0f + (port * 2), 0x01, 0x02 } };
	for (int i = 2; i < 54 * fan_cnt; i+=3) {
		data[1][i]     = r;
		data[1][i + 1] = b;
		data[1][i + 2] = g;
	}
	for (int i = 0; i < 4; i++) {
		memcpy(mbuff, data[i], datalen);
		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, data[i], datalen, 1000);
		if (ret != datalen) printk("Lian Li Hub - failed to send usb_control_msg with data[%d] err: %d", i, ret);
	}

	kfree(mbuff);
}

void set_inner_color(int r, int g, int b, int port, int fan_cnt)
{
	int datalen = 353;
	unsigned char *mbuff = kcalloc(datalen, sizeof(unsigned char), GFP_KERNEL);
	unsigned char data[4][353] = { { 0xe0, 0x10, 0x60, port, fan_cnt }, { 0xe0, 0x2e + (port * 2) },
					{ 0xe0, 0x0e + (port * 2), 0x01, 0x02 }, { 0xe0, 0x0f + (port * 2), 0x01, 0x02 } };

	for (int i = 2; i < 24 * fan_cnt; i += 3) {
		data[1][i]     = r;
		data[1][i + 1] = b;
		data[1][i + 2] = g;
	}

	for (int i = 0; i < 3; i++) {
		memcpy(mbuff, data[i], datalen);
		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, data[i], datalen, 1000);
		if (ret != datalen) printk("Lian Li Hub - failed to send usb_control_msg with data[%d] err: %d", i, ret);
	}

	kfree(mbuff);
}

static void set_speed(int new_speeds[])
{
	int datalen = 353;
	unsigned char *mbuff = kcalloc(datalen, sizeof(unsigned char), GFP_KERNEL);
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

static void get_speed(void)
{
	int response_len = 65;
	unsigned char *response = kcalloc(response_len, 1, GFP_KERNEL);
	int ret = usb_control_msg_recv(dev, 0x80, 0x01, 0xa1, 0x01e0, 1, response, response_len, 1000, GFP_KERNEL);
	if (ret != response_len) printk("Lian Li Hub - failed to get speed err: %d", ret);
	ports[0].fan_speed_rpm = ((response[2] << 8) | response[3]);
	ports[1].fan_speed_rpm = ((response[4] << 8) | response[5]);
	ports[2].fan_speed_rpm = ((response[6] << 8) | response[7]);
	ports[3].fan_speed_rpm = ((response[8] << 8) | response[9]);
	kfree(response);
}


static ssize_t read_fan_speed(struct file *f, char *ubuf, size_t count, loff_t *offs)
{
	char *text= kcalloc(32, sizeof(char), GFP_KERNEL);
	int to_copy, not_copied, delta;

	to_copy = min(count, 32);

	sprintf(text, "%d %d %d %d\n", ports[0].fan_speed, ports[1].fan_speed, ports[2].fan_speed, ports[3].fan_speed);
	
	not_copied = copy_to_user(ubuf, text, to_copy);

	//i have no idea what this does but if i dont do it and try to read it doesnt stop printing
	if (*offs) return 0;
	else *offs = 32;

	delta = to_copy - not_copied;

	kfree(text);
	return delta;
}

static ssize_t write_fan_speed(struct file *f, const char *ubuf, size_t count, loff_t *offs)
{
	char *text= kcalloc(32, sizeof(char), GFP_KERNEL);
	int to_copy, not_copied, delta;
	int new_speed[4];

	to_copy = min(count, 32);

	not_copied = copy_from_user(text, ubuf, to_copy);

	sscanf(text, "%d %d %d %d", &new_speed[0], &new_speed[1], &new_speed[2], &new_speed[3]);

	set_speed(new_speed);

	delta = to_copy - not_copied;

	kfree(text);
	return delta;
}

static ssize_t read_rgb(struct file *f, char *ubuf, size_t count, loff_t *offs)
{
	char *text = kcalloc(363, sizeof(char), GFP_KERNEL);
	text = "hello";
	int to_copy, not_copied, delta;

	to_copy = min(count, 363);

	not_copied = copy_to_user(ubuf, text, to_copy);
	
	//i have no idea what this does but if i dont do it and try to read it doesnt stop printing
	if(*offs) return 0;
	else *offs = 363;

	delta = to_copy - not_copied;

	kfree(text);
	return delta;
}

static ssize_t write_rgb(struct file *f, const char *ubuf, size_t count, loff_t *offs)
{
	char *text = kcalloc(363, sizeof(char), GFP_KERNEL);
	int to_copy, not_copied, delta;
	
	to_copy = min(count, 363);

	not_copied = copy_from_user(text, ubuf, to_copy);

	delta = to_copy - not_copied;

	return delta;
}

static struct proc_ops pops_fan_speed = {
	.proc_read = read_fan_speed,
	.proc_write = write_fan_speed,
};

static struct proc_ops pops_rgb = {
	.proc_read = read_rgb,
	.proc_write = write_rgb,
};

static int dev_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	printk("Lian Li Hub - driver probe\n");

	dev = interface_to_usbdev(intf);
	if (dev == NULL) {
		printk("Lian Li Hub - error getting dev from intf");
		return -1;
	}

	proc_dir = proc_mkdir("Lian_Li_Hub", NULL);
	if (proc_dir == NULL) {
		printk("Lian Li Hub - error proc_create dir failed");
		return -1;
	}

	proc_fan_speed = proc_create("fan_speeds", 0666, proc_dir, &pops_fan_speed);
	if (proc_fan_speed == NULL) {
		printk("Lian Li Hub - error proc_create fan speed failed");
		return -1;
	}

	proc_rgb = proc_create("rgb", 0666, proc_dir, &pops_rgb);
	if (proc_rgb == NULL) {
		printk("Lian Li Hub - error proc_create rgb failed");
		return -1;
	}

	int defaultspeeds[] = { 30, 30, 40, 0 };
	//get_speed();
	set_speed(defaultspeeds);

	return 0;
}

static void dev_disconnect(struct usb_interface *intf)
{
	proc_remove(proc_fan_speed);
	proc_remove(proc_rgb);
	proc_remove(proc_dir);
	printk("Lian Li Hub - driver disconnect\n");
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
		printk("Lian Li Hub - Error during register\n");
		return -res;
	}
	return 0;
}

static void __exit controller_exit(void)
{
	printk("Lian Li Hub - driver exit\n");
	usb_deregister(&driver);
}

module_init(controller_init);
module_exit(controller_exit);
