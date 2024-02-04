#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/usb.h>

#define VENDOR_ID 0x0cf2
#define PRODUCT_ID 0xa104

MODULE_LICENSE("GPL");

struct rgb_s {
	int mode, brightnes, speed, direction;
};

struct port_s {
	int fanSpeed, fanCount;
	struct rgb_s rgb;
};

static struct port_s ports[4];

static struct usb_device *dev;
static struct proc_dir_entry *proc_file;

static struct usb_device_id dev_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{},
};
MODULE_DEVICE_TABLE(usb, dev_table);

void setOuterColor(int r, int g, int b, int port, int fanCount)
{
	int datalen = 353;
	unsigned char *mbuff = kmalloc(datalen, GFP_KERNEL);
	unsigned char data[4][353] = { { 0xe0, 0x10, 0x60, port, fanCount }, { 0xe0, 0x2f + (port * 2) }, 
								   { 0xe0, 0x0e + (port * 2), 0x01, 0x02 }, { 0xe0, 0x0f + (port * 2), 0x01, 0x02 } };
	for (int i = 2; i < 54 * fanCount; i+=3) {
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

void setInnerColor(int r, int g, int b, int port, int fanCount)
{
	int datalen = 353;
	unsigned char *mbuff = kmalloc(datalen, GFP_KERNEL);
	unsigned char data[4][353] = { { 0xe0, 0x10, 0x60, port, fanCount }, { 0xe0, 0x2e + (port * 2) },
								   { 0xe0, 0x0e + (port * 2), 0x01, 0x02 }, { 0xe0, 0x0f + (port * 2), 0x01, 0x02 } };
	for (int i = 2; i < 24 * fanCount; i += 3) {
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

static void setSpeed(int newSpeeds[])
{
	int datalen = 353;
	unsigned char *mbuff = kmalloc(353, GFP_KERNEL);
	unsigned char data[5][353] = { { 0xe0, 0x50, }, { 0xe0, 0x20, 0x00, newSpeeds[0], }, 
								{ 0xe0, 0x21, 0x00, newSpeeds[1], }, { 0xe0, 0x22, 0x00, newSpeeds[2], }, { 0xe0, 0x23, 0x00, newSpeeds[3], } };
	for (int i = 0; i < 5; i++) {
		memcpy(mbuff, data[i], 353);
		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, mbuff, datalen, 100);
		if (ret != datalen) {
			printk("Lian Li Hub - failed to send usb_control_msg with data[%d] err: %d", i, ret);
		} else if (i >= 1) {
			ports[i-1].fanSpeed = newSpeeds[i-1];
		}
	}

	kfree(mbuff);
	printk("Lian Li Hub - speed set to %d %d %d %d", ports[0].fanSpeed, ports[1].fanSpeed, ports[2].fanSpeed, ports[3].fanSpeed);
}

static ssize_t read(struct file *f, char *ubuf, size_t count, loff_t *offs)
{
	char *text = kmalloc(32, GFP_KERNEL);
	int tocopy, notcopied, delta;

	tocopy = min(count, 32);

	sprintf(text, "%d %d %d %d\n", ports[0].fanSpeed, ports[1].fanSpeed, ports[2].fanSpeed, ports[3].fanSpeed);
	
	notcopied = copy_to_user(ubuf, text, tocopy);

	kfree(text);
	delta = tocopy - notcopied;
	if (*offs) return 0;
	else *offs = 32;

	return delta;
}

static ssize_t write(struct file *f, const char *ubuf, size_t count, loff_t *offs)
{
	char *text = kmalloc(32, GFP_KERNEL);
	int tocopy, notcopied, delta;
	int newspeeds[4];

	memset(text, 0, 32);

	tocopy = min(count, 32);

	notcopied = copy_from_user(text, ubuf, tocopy);

	sscanf(text, "%d %d %d %d", &newspeeds[0], &newspeeds[1], &newspeeds[2], &newspeeds[3]);

	setSpeed(newspeeds);

	delta = tocopy - notcopied;

	return delta;
}

static struct proc_ops fops = {
	.proc_read = read,
	.proc_write = write,
};

static int dev_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	printk("Lian Li Hub - driver probe\n");

	dev = interface_to_usbdev(intf);
	if (dev == NULL) {
		printk("Lian Li Hub - error getting dev from intf");
		return -1;
	}

	proc_file = proc_create("LianLiALHub", 0666, NULL, &fops);
	if (proc_file == NULL) {
		printk("Lian Li Hub - error proc_create failed");
		return -1;
	}
	
	int defaultspeeds[] = { 30, 30, 40, 0 };
	setSpeed(defaultspeeds);

	return 0;
}

static void dev_disconnect(struct usb_interface *intf)
{
	proc_remove(proc_file);
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
