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
static int set_speed(struct drv_data *drv, int channel, long val);

static int mb_sync(struct drv_data *drv, int val);


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
	u8 port = --chanel + 0x20;
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

	drv->speed[chanel] = speed;
out:
	return ret;
}

// old:
//    usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, buffer, PACKET_SIZE, 100);
static int mb_sync(struct drv_data *drv, int val)
{
	u8 buffer[] = { MSG_START, 0x10, 0x61, val };

	int ret = send_report(drv, buffer, sizeof(buffer));
	if (ret) return ret;

	mb_sync_state = val;
	return ret;
}

static int uni_probe(struct hid_device *hid_dev, const struct hid_device_id *hid_dev_id)
{
	struct drv_data *drv = NULL;
	drv = devm_kzalloc(&hid_dev->dev, sizeof(struct drv_data), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	drv->dev = hid_dev;
	hid_set_drvdata(hid_dev, drv);

	int ret = hid_parse(hid_dev);
	if (ret) return ret;

	ret = hid_hw_start(hid_dev, HID_CONNECT_HIDRAW);
	if (ret) return ret;

	ret = hid_hw_open(hid_dev);
	if (ret) goto out_stop_hw;

	hid_device_io_start(hid_dev);
	hwmon_device_register_with_info(&hid_dev->dev, "uni hub ALv2", drv, const struct hwmon_chip_info *info, const struct attribute_group **extra_groups);

	return 0;

out_stop_hw:
	hid_hw_stop(hid_dev);
	return ret;
}

const struct hwmon_channel_info *uni_hub_alv2_channel[] = {
	HWMON_CHANNEL_INFO(fan, HWMON_F_INPUT | HWMON_F_LABEL,
			   HWMON_F_INPUT | HWMON_F_LABEL,
			   HWMON_F_INPUT | HWMON_F_LABEL),
	HWMON_CHANNEL_INFO(pwm, HWMON_PWM_INPUT | HWMON_PWM_MODE | HWMON_PWM_ENABLE,
			   HWMON_PWM_INPUT | HWMON_PWM_MODE | HWMON_PWM_ENABLE,
			   HWMON_PWM_INPUT | HWMON_PWM_MODE | HWMON_PWM_ENABLE),
	HWMON_CHANNEL_INFO(in, HWMON_I_INPUT | HWMON_I_LABEL,
			   HWMON_I_INPUT | HWMON_I_LABEL,
			   HWMON_I_INPUT | HWMON_I_LABEL),
	HWMON_CHANNEL_INFO(curr, HWMON_C_INPUT | HWMON_C_LABEL,
			   HWMON_C_INPUT | HWMON_C_LABEL,
			   HWMON_C_INPUT | HWMON_C_LABEL),
	HWMON_CHANNEL_INFO(chip, HWMON_C_UPDATE_INTERVAL),
	NULL
};
static umode_t uni_alv2_hwmon_is_visible(const void *data,
					 enum hwmon_sensor_types type,
					 u32 attr, int channel);

static int uni_alv2_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
			       u32 attr, int channel, long *val)
{
	struct drvdata *drvdata = dev_get_drvdata(dev);
	int res = -EINVAL;

	switch (type) {
	case hwmon_pwm:
		switch (attr) {
		case hwmon_pwm_enable:
			*val = drvdata->fan_type[channel] != FAN_TYPE_NONE;
			break;

		case hwmon_pwm_mode:
			res = wait_event_interruptible_locked_irq(drvdata->wq,
								  drvdata->fan_config_received);
			if (res)
				goto unlock;

			*val = drvdata->fan_type[channel] == FAN_TYPE_PWM;
			break;

		case hwmon_pwm_input:
			res = wait_event_interruptible_locked_irq(drvdata->wq,
								  drvdata->pwm_status_received);
			if (res)
				goto unlock;

			*val = scale_pwm_value(drvdata->fan_duty_percent[channel],
					       100, 255);
			break;
		}
		break;

	case hwmon_fan:
		/*
		 * It's not strictly necessary to wait for *_received in the
		 * remaining cases (fancontrol doesn't care about them). But I'm
		 * doing it to have consistent behavior.
		 */
		if (attr == hwmon_fan_input) {
			res = wait_event_interruptible_locked_irq(drvdata->wq,
								  drvdata->pwm_status_received);
			if (res)
				goto unlock;

			*val = drvdata->fan_rpm[channel];
		}
		break;

	case hwmon_in:
		if (attr == hwmon_in_input) {
			res = wait_event_interruptible_locked_irq(drvdata->wq,
								  drvdata->voltage_status_received);
			if (res)
				goto unlock;

			*val = drvdata->fan_in[channel];
		}
		break;

	case hwmon_curr:
		if (attr == hwmon_curr_input) {
			res = wait_event_interruptible_locked_irq(drvdata->wq,
								  drvdata->voltage_status_received);
			if (res)
				goto unlock;

			*val = drvdata->fan_curr[channel];
		}
		break;

	default:
		break;
	}

}

static struct hwmon_ops ops = {
	.is_visible = uni_alv2_hwmon_is_visible,
	.read = uni_alv2_hwmon_read,
};
static struct hwmon_chip_info uni_hub_alv2_info = {
	
};

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
