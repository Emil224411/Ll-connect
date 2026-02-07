#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/hid.h>
#include <linux/hwmon.h>
#include <linux/module.h>
#include <linux/string.h>

#define VENDOR_ID  0x0cf2
#define PRODUCT_ID 0xa104

#define BUFFER_SIZE 353
#define RCV_BUF_SIZE 65

#define MSG_START 0xe0

#define SET_SPEED 0x50

#define PORT_AMOUNT 4
#define PORT_ONE   0x20
#define PORT_TWO   0x21
#define PORT_THREE 0x22
#define PORT_FOUR  0x23

#define DPRINTF(fmt, ...) printk(KERN_DEBUG "ALv2: "fmt"\n", ##__VA_ARGS__)
struct header {
	u8 magic;
	u16 type;
};

struct intf_data {
	struct usb_device *udev;
	struct device *hwmon;
	struct usb_interface *intf;

	u16 rpm[PORT_AMOUNT];
	u8 pwm[PORT_AMOUNT];
	// u8 mb_sync;

	struct mutex lock;

	u8 buffer[BUFFER_SIZE];
};


static int set_speed(struct intf_data *drv, int channel, long val);

// static int mb_sync(struct intf_data *drv, int val);


// static void set_speeds(int port_one, int port_two, int port_three, int port_four) {
// 	int new_speeds[] = { port_one, port_two, port_three, port_four };
// 	u8 data[5][BUFFER_SIZE] =  { { 0xe0, 0x50, }, 
// 				{ 0xe0, 0x20, 0x00, new_speeds[0], }, 
// 				{ 0xe0, 0x21, 0x00, new_speeds[1], }, 
// 				{ 0xe0, 0x22, 0x00, new_speeds[2], }, 
// 				{ 0xe0, 0x23, 0x00, new_speeds[3], } };
// 	for (int i = 0; i < 5; i++) {
// 		memcpy(mbuff, data[i], PACKET_SIZE);
// 		usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, mbuff, PACKET_SIZE, 100);
// 		ports[i-1].fan_speed = new_speeds[i-1];
// 	}
// }
static int send_usb_msg(struct intf_data *drv, 
			u8 req, 
			u8 req_t, 
			u16 val, 
			u16 idx, 
			u8 rcv,
			const void *data, 
			size_t data_size) 
{
	int ret;
	if (data_size > BUFFER_SIZE) 
		return -EINVAL;

	memset(drv->buffer, 0, BUFFER_SIZE);
	memcpy(drv->buffer, data, data_size);
	
	ret = usb_control_msg(drv->udev,
			      rcv ? usb_rcvctrlpipe(drv->udev, USB_DIR_IN) 
			          : usb_sndctrlpipe(drv->udev, USB_DIR_OUT), 
			      req, req_t, 
			      val, idx, 
			      drv->buffer, 
			      BUFFER_SIZE, 
			      100);
	return ret;
}

/*
 * Hub only sends all the fan rpm's at the same time,
 * Meaning there is no way to get a specific fan's speed.
 */
static int update_rpm(struct intf_data *data)
{
	/* gets rpm from hub */

	int res = send_usb_msg(data, 0x01, 0xa1, 0x01e0, 1, /* TODO figure out the magic numbers */
			       USB_DIR_IN, data->buffer, RCV_BUF_SIZE);
	if (res < 0) return res;
	data->rpm[0] = (data->buffer[2] << 8) + data->buffer[3];
	data->rpm[1] = (data->buffer[4] << 8) + data->buffer[5];
	data->rpm[2] = (data->buffer[6] << 8) + data->buffer[7];
	data->rpm[3] = (data->buffer[8] << 8) + data->buffer[9];
	return 0;
}

#define SET_CMD 0x02e0
static int set_speed(struct intf_data *drv, int channel, long val)
{
	if (val < 0 || val > 255)
		return -EINVAL;
	
	u8 speed = max(1L, DIV_ROUND_CLOSEST(val * 100, 255)); // 1 <= speed <= 100
	DPRINTF("speed: %u, %ld", speed, val);

	/* hub uses port numbers 0x20-0x23 */
	u8 port = channel + 0x20;

	u8 header[] = { MSG_START, SET_SPEED };
	u8 body[] = { MSG_START, port, 0x00, speed }; 

	int ret;
	ret = send_usb_msg(drv, 
			   USB_REQ_SET_CONFIGURATION, 
			   USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
			   SET_CMD, 
			   1, 
			   USB_DIR_OUT, &header, sizeof(header));
	if (ret < 0) return ret;
	ret = send_usb_msg(drv, 
			   USB_REQ_SET_CONFIGURATION, 
			   USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
			   SET_CMD, 
			   1,
			   USB_DIR_OUT, &body, sizeof(body));
	if (ret < 0) return ret;

	drv->pwm[channel] = val;
	return 0;
}

// This is most likly not correct, but i dont use it and i dont really want to figure it out again so.... TODO
// static int mb_sync(struct intf_data *drv, int val)
// {
// 	u8 buffer[] = { MSG_START, 0x10, 0x61, val ? 1 : 0 };

// 	int ret = send_usb_msg(drv, 0x09, 0x21, 0x02e0, 1, 0, buffer, sizeof(buffer));
// 	if (ret < 0) return ret;

// 	drv->mb_sync = val;
// 	return 0;
// }

const struct hwmon_channel_info *uni_hub_alv2_channel_info[] = {
	HWMON_CHANNEL_INFO(fan, 
			   HWMON_F_INPUT, HWMON_F_INPUT,
			   HWMON_F_INPUT, HWMON_F_INPUT),
	HWMON_CHANNEL_INFO(pwm, 
			   HWMON_PWM_INPUT, HWMON_PWM_INPUT,
			   HWMON_PWM_INPUT, HWMON_PWM_INPUT),
	NULL
};
static umode_t uni_alv2_hwmon_is_visible(const void *data,
					 enum hwmon_sensor_types type,
					 u32 attr, int channel)
{
	switch (type) {
	case hwmon_fan:
		switch (attr) {
		case hwmon_fan_input:
			return 0444;
		case hwmon_fan_label:
			return 0444;
		default:
			break;
		}
		break;
	case hwmon_pwm:
		switch (attr) {
		case hwmon_pwm_input:
			return 0644;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return 0;
}

static int uni_alv2_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
			       u32 attr, int channel, long *val)
{
	struct intf_data *data = dev_get_drvdata(dev);
	int ret;

	switch (type) {
	case hwmon_fan:
		if (attr == hwmon_fan_input) {
			ret = update_rpm(data);
			if (ret) return ret;
			*val = data->rpm[channel];
			return 0;
		}
		break;
	case hwmon_pwm:
		if (attr == hwmon_pwm_input) {
			*val = data->pwm[channel];
			return 0;
		}
		break;
	default:
		break;
	}

	return -EOPNOTSUPP;
}

static int uni_alv2_hwmon_write(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long val)
{
	struct intf_data *data = dev_get_drvdata(dev);
	DPRINTF("writeing: attr: %d, channel: %d, val: %ld", attr, channel, val);
	switch (type) {
	case hwmon_pwm:
		if (attr == hwmon_pwm_input) {
			 return set_speed(data, channel, val);
		}
		break;
	default:
		break;
	}
	return -EOPNOTSUPP;
}

static const struct hwmon_ops alv2_hwmon_ops = {
	.is_visible = uni_alv2_hwmon_is_visible,
	.read = uni_alv2_hwmon_read,
	.write = uni_alv2_hwmon_write,
	
};

static void alv2_disconnect(struct usb_interface *intf)
{
	struct intf_data *data = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);

	usb_put_dev(data->udev);
}

static struct hwmon_chip_info alv2_hwmon_chip_info = {
	.ops = &alv2_hwmon_ops,
	.info = uni_hub_alv2_channel_info
};

static int alv2_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	DPRINTF("probe: id intf num: %d", id->bInterfaceNumber);
	struct intf_data *drv = NULL;

	drv = devm_kzalloc(&intf->dev, sizeof(*drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	drv->udev = usb_get_dev(interface_to_usbdev(intf));
	drv->intf = intf;

	mutex_init(&drv->lock);

	usb_set_intfdata(intf, drv);

	drv->hwmon = devm_hwmon_device_register_with_info(&intf->dev, "hub",
						          drv, &alv2_hwmon_chip_info, NULL);

	if (IS_ERR(drv->hwmon)) {
		return PTR_ERR(drv->hwmon);
	}
	DPRINTF("probe done");
	return 0;
}

static struct usb_device_id alv2_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{},
};
MODULE_DEVICE_TABLE(usb, alv2_table);

static struct usb_driver alv2_drv = {
	.name = "uni-hub-ALv2",
	.id_table = alv2_table,
	.disconnect = alv2_disconnect,
	.probe = alv2_probe,
};

static int __init mod_init(void)
{
	DPRINTF("mod_init");
	int res;
	res = usb_register(&alv2_drv);
	DPRINTF("usb_register: %d", res);
	if (res) {
		printk(KERN_ERR "Lian li ALv2 hub: Error during register\n");
		return -res;
	}
	DPRINTF("init over");
	return res;
}

static void __exit mod_exit(void)
{
	DPRINTF("we out");
	usb_deregister(&alv2_drv);
}


module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Emil <emil@svansoe.io>");
MODULE_DESCRIPTION("driver for UNI HUB ALv2");
MODULE_LICENSE("GPL");
