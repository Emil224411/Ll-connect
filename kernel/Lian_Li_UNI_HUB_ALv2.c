#include <linux/hid.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/hwmon.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/jiffies.h>

#define DPRINTF(fmt, ...) printk(KERN_DEBUG "ALv2: "fmt"\n", ##__VA_ARGS__)

#define VENDOR_ID  0x0cf2
#define PRODUCT_ID 0xa104

// maybe change
#define BUFFER_SIZE 353
#define FAN_SPEED_REPORT_SIZE 65

#define REPORT_ID 0xe0

#define MSG_START 0xe0

#define SET_SPEED 0x50

#define ALV2_NUM_PORTS 4

#define DISABLE_MODE 0
#define MANUAL_MODE 1
#define AUTO_MODE 2


static const char * const port_label[] = { 
	"Port 1", 
	"Port 2",
	"Port 3",
	"Port 4",
};

struct drv_data {
	struct hid_device *hid_dev;
	struct device *hwmon;

	u16 rpm[ALV2_NUM_PORTS];
	bool valid;
	unsigned long last_update;
	u8 pwm[ALV2_NUM_PORTS];
	u8 enable[ALV2_NUM_PORTS];
	// u8 mb_sync;

	struct mutex lock;

	u8 buffer[BUFFER_SIZE];
};

// This is most likly not correct, but i dont use it and i dont really want to figure it out again so.... TODO
// static int mb_sync(struct intf_data *drv, int val)
// {
// 	u8 buffer[] = { MSG_START, 0x10, 0x61, val ? 1 : 0 };
//
// 	int ret = send_usb_msg(drv, 0x09, 0x21, 0x02e0, 1, 0, buffer, sizeof(buffer));
// 	if (ret < 0) return ret;
//
// 	drv->mb_sync = val;
// 	return 0;
// }

// NOTE maybe useful later for batching set_speed
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


static int send_output_report(struct drv_data *drv, 
			      const void *data, 
			      size_t data_size)
{
	int ret;

	if (data_size > BUFFER_SIZE) 
		return -EINVAL;

	memcpy(drv->buffer, data, data_size);
	if (data_size < BUFFER_SIZE) 
		memset(drv->buffer + data_size, 0, BUFFER_SIZE-data_size);
	
	// NOTE maybe this should be changed after sniffing traffic it -
	// - seems to not just be a static buffer size, tho it works :/
	// - rgb packets are BUFFER_SIZE, fan speed and the others are different
	ret = hid_hw_raw_request(drv->hid_dev,
				 REPORT_ID,
				 drv->buffer,
				 BUFFER_SIZE,
				 HID_OUTPUT_REPORT,
				 HID_REQ_SET_REPORT);
	return ret < 0 ? ret : 0;
}

static int set_mb_sync(struct drv_data *drv, int channel, int enable)
{
	u8 port;
	u8 data[7] = {0};

	if (channel < 0 || channel > 3)
		return -EINVAL;
	
	port = channel + 1;
	
	// TODO make label magic numbers
	data[0] = MSG_START;
	data[1] = 0x10;
	data[2] = 0x62;
	data[3] = 8 << port | (enable ? 1 : 0) << (port - 1);

	return send_output_report(drv, &data, sizeof(data));
}

/*
 * Hub only sends all the fan rpm's at the same time,
 * Meaning there is no way to get a specific fan's speed.
 * TODO change the timeout period
 */
static int update_rpm(struct drv_data *drv_data)
{
	int res;

	if (drv_data->valid && time_before(jiffies, drv_data->last_update + HZ/4))
		return 0;

	res = hid_hw_raw_request(drv_data->hid_dev,
				 REPORT_ID,
				 drv_data->buffer,
				 FAN_SPEED_REPORT_SIZE,
				 HID_INPUT_REPORT,
				 HID_REQ_GET_REPORT);
	if (res < 10) 
		return res < 0 ? res : -EIO;

	drv_data->rpm[0] = (drv_data->buffer[2] << 8) | drv_data->buffer[3];
	drv_data->rpm[1] = (drv_data->buffer[4] << 8) | drv_data->buffer[5];
	drv_data->rpm[2] = (drv_data->buffer[6] << 8) | drv_data->buffer[7];
	drv_data->rpm[3] = (drv_data->buffer[8] << 8) | drv_data->buffer[9];
	drv_data->last_update = jiffies;
	drv_data->valid = true;

	return 0;
}

static int set_speed(struct drv_data *drv, int channel, long val)
{
	int ret;
	u8 speed;
	u8 body[4];
	u8 header[] = { MSG_START, SET_SPEED, };

	if (val < 0 || val > 255)
		return -EINVAL;

	speed = max(1L, DIV_ROUND_CLOSEST(val * 100, 255));

	body[0] = MSG_START; 
	/* hub uses port numbers 0x20-0x23 */
	body[1] = channel + 0x20;
	body[2] = 0x00; 
	body[3] = speed; 

	ret = send_output_report(drv, &header, sizeof(header));
	if (ret < 0) 
		return ret;
	ret = send_output_report(drv, &body, sizeof(body));
	if (ret < 0) 
		return ret;

	return 0;
}

static int set_pwm_enable(struct drv_data *drv, int channel, long val)
{
	int ret;

	mutex_lock(&drv->lock);

	// if val == AUTO_MODE then toggle motherboard sync on else toggle it off
	// maybe check be for toggling
	ret = set_mb_sync(drv, channel, val == AUTO_MODE);
	if (ret != 0)
		goto out;

	if (val == DISABLE_MODE)
		ret = set_speed(drv, channel, 255);
	else if (val == MANUAL_MODE)
		ret = set_speed(drv, channel, drv->pwm[channel]);

	if (ret != 0)
		goto out;

	drv->enable[channel] = val;
out:
	mutex_unlock(&drv->lock);
	return ret;
}

static int set_pwm(struct drv_data *drv, int channel, long val)
{
	int ret = 0;

	mutex_lock(&drv->lock);

	// Only set speed if mode is manual 
	// if mode is not manual we just update the pwm value
	if (drv->enable[channel] == MANUAL_MODE) {
		ret = set_speed(drv, channel, val);
		if (ret != 0) goto out;
	}

	drv->pwm[channel] = val;

out:
	mutex_unlock(&drv->lock);
	return ret;
}

static const struct hwmon_channel_info * const uni_hub_alv2_channel_info[] = {
	HWMON_CHANNEL_INFO(fan, 
			   HWMON_F_INPUT | HWMON_F_LABEL,
			   HWMON_F_INPUT | HWMON_F_LABEL,
			   HWMON_F_INPUT | HWMON_F_LABEL,
			   HWMON_F_INPUT | HWMON_F_LABEL),
	HWMON_CHANNEL_INFO(pwm, 
			   HWMON_PWM_INPUT | HWMON_PWM_ENABLE,
			   HWMON_PWM_INPUT | HWMON_PWM_ENABLE,
			   HWMON_PWM_INPUT | HWMON_PWM_ENABLE,
			   HWMON_PWM_INPUT | HWMON_PWM_ENABLE),
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
		case hwmon_pwm_enable:
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
	struct drv_data *drv = dev_get_drvdata(dev);
	int ret;

	if (channel < 0 || channel >= ALV2_NUM_PORTS)
		return -EINVAL;

	switch (type) {
	case hwmon_fan:
		if (attr == hwmon_fan_input) {
			mutex_lock(&drv->lock);

			ret = update_rpm(drv);
			if (ret == 0) 
				*val = drv->rpm[channel];

			mutex_unlock(&drv->lock);

			return ret;
		}
		break;
	case hwmon_pwm:
		switch (attr) {
		case hwmon_pwm_input:
			*val = drv->pwm[channel];
			return 0;
		case hwmon_pwm_enable:
			*val = drv->enable[channel];
			return 0;
		default:
			break;
		}
	default:
		break;
	}

	return -EOPNOTSUPP;
}

static int uni_alv2_hwmon_write(struct device *dev, 
				enum hwmon_sensor_types type, 
				u32 attr, int channel, long val)
{
	struct drv_data *drv = dev_get_drvdata(dev);

	if (channel < 0 || channel >= ALV2_NUM_PORTS)
		return -EINVAL;

	switch (type) {
	case hwmon_pwm:
		switch (attr) {
		case hwmon_pwm_input:
			return set_pwm(drv, channel, val);
		case hwmon_pwm_enable:
			return set_pwm_enable(drv, channel, val);
		default:
			return -EOPNOTSUPP;
		}
		break;
	default:
		return -EOPNOTSUPP;
	}
}

static int uni_alv2_hwmon_read_string(struct device *dev,
				      enum hwmon_sensor_types type, 
				      u32 attr, int channel, const char **str)
{
	if (channel < 0 || channel >= ALV2_NUM_PORTS)
		return -EINVAL;

	switch (type) {
	case hwmon_fan:
		if (attr == hwmon_fan_label) {
			*str = port_label[channel];
			return 0;
		} else {
			return -EOPNOTSUPP;
		}
	default:
		return -EOPNOTSUPP;
	}
}


static const struct hwmon_ops alv2_hwmon_ops = {
	.is_visible  = uni_alv2_hwmon_is_visible,
	.read        = uni_alv2_hwmon_read,
	.read_string = uni_alv2_hwmon_read_string,
	.write       = uni_alv2_hwmon_write,
};

static const struct hwmon_chip_info alv2_hwmon_chip_info = {
	.ops = &alv2_hwmon_ops,
	.info = uni_hub_alv2_channel_info
};

static void alv2_hid_remove(struct hid_device *hid_dev)
{
	// hid_hw_close(hid_dev);
	hid_hw_stop(hid_dev);
}

static void mutex_des(void *mutex)
{
	mutex_destroy(mutex);
}

static int alv2_hid_probe(struct hid_device *hid_dev, const struct hid_device_id *id)
{
	struct drv_data *drv = NULL;
	int ret = 0;

	drv = devm_kzalloc(&hid_dev->dev, sizeof(*drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	drv->hid_dev = hid_dev;
	hid_set_drvdata(hid_dev, drv);

	mutex_init(&drv->lock);
	ret = devm_add_action_or_reset(&hid_dev->dev, mutex_des, &drv->lock);
	if (ret) return ret;

	ret = hid_parse(hid_dev);
	if (ret) return ret;

	ret = hid_hw_start(hid_dev, HID_CONNECT_HIDRAW);
	if (ret) return ret;

	drv->hwmon = devm_hwmon_device_register_with_info(&hid_dev->dev, "uni_hub_alv2",
						          drv, &alv2_hwmon_chip_info, NULL);

	if (IS_ERR(drv->hwmon)) {
		hid_hw_stop(hid_dev);
		return PTR_ERR(drv->hwmon);
	}
	return 0;
}

static const struct hid_device_id alv2_table[] = {
	{ HID_USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{},
};
MODULE_DEVICE_TABLE(hid, alv2_table);

static struct hid_driver alv2_drv = {
	.name = "uni-hub-ALv2",
	.id_table = alv2_table,
	.remove   = alv2_hid_remove,
	.probe    = alv2_hid_probe,
	
};

static int __init mod_init(void)
{
	return hid_register_driver(&alv2_drv);
}

static void __exit mod_exit(void)
{
	hid_unregister_driver(&alv2_drv);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Emil <emil@svansoe.io>");
MODULE_DESCRIPTION("driver for UNI HUB ALv2");
MODULE_LICENSE("GPL");
