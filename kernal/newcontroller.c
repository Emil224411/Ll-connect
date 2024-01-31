#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/usb.h>

#define VENDOR_ID 0x0cf2
#define PRODUCT_ID 0xa104

MODULE_LICENSE("GPL");

struct hub {
	int speeds[4];
};

static struct hub speedrn;

static struct usb_device *dev;
static struct proc_dir_entry *proc_file;

static struct usb_device_id dev_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{},
};
MODULE_DEVICE_TABLE(usb, dev_table);


static void setSpeed(struct hub newspeed)
{
	int datalen = 353;
	unsigned char *mbuff = kmalloc(353, GFP_KERNEL);
	unsigned char data[5][353] = { { 0xe0, 0x50, }, { 0xe0, 0x20, 0x00, newspeed.speeds[0], }, 
								{ 0xe0, 0x21, 0x00, newspeed.speeds[1], }, { 0xe0, 0x22, 0x00, newspeed.speeds[2], }, { 0xe0, 0x23, 0x00, newspeed.speeds[3], } };
	for (int i = 0; i < 5; i++) {
		memcpy(mbuff, data[i], 353);
		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, mbuff, datalen, 100);
		printk("%d", ret);
	}

	for (int i = 0; i < 4; i++) speedrn.speeds[i] = newspeed.speeds[i];

	kfree(mbuff);

	return;
}

static ssize_t read(struct file *f, char *ubuf, size_t count, loff_t *offs)
{
	char *text = kmalloc(32, GFP_KERNEL);
	int tocopy, notcopied, delta;

	tocopy = min(count, sizeof(text));
	
	sprintf(text, "%d %d %d %d", speedrn.speeds[0], speedrn.speeds[1], speedrn.speeds[2], speedrn.speeds[3]);
	
	notcopied = copy_to_user(ubuf, text, tocopy);

	delta = tocopy-notcopied;
	return delta;
	
}

static ssize_t write(struct file *f, const char *ubuf, size_t count, loff_t *offs)
{
	char *text = kmalloc(32, GFP_KERNEL);
	int tocopy, notcopied, delta;
	struct hub newspeed;

	memset(text, 0, 32);

	tocopy = min(count, 32);

	notcopied = copy_from_user(text, ubuf, tocopy);

	sscanf(text, "%d %d %d %d", &newspeed.speeds[0], &newspeed.speeds[1], &newspeed.speeds[2], &newspeed.speeds[3]);

	setSpeed(newspeed);

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
	/*int status;
	unsigned char *data = kmalloc(sizeof(unsigned char)*64, GFP_KERNEL);
	printk("%ld", sizeof(unsigned char));
	data[0] = 0x01;
	data[1] = 0x02;
	printk("%ld", sizeof(data));
	status = usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, data, 64, 100);
	if (status < 0) {
		printk("Lian Li Hub - error sending msg, %d", status);
		kfree(data);
		return -1;
	}
	kfree(data);*/
	speedrn.speeds[0] = 25;
	speedrn.speeds[1] = 25;
	speedrn.speeds[2] = 30;
	speedrn.speeds[3] = 0;
	setSpeed(speedrn);

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
