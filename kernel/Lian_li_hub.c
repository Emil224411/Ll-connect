#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/thermal.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/usb.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

#define VENDOR_ID 0x0cf2
#define PRODUCT_ID 0xa104

#define PACKET_SIZE 353

#define INNER 		0b00000001
#define OUTER           0b00000010
#define INNER_AND_OUTER 0b00000100
#define MERGE           0b00001000
#define NOT_MOVING      0b00010000
#define BRIGHTNESS      0b00100000
#define SPEED           0b01000000
#define DIRECTION       0b10000000

struct rgb_data {
	int mode, brightness, speed, direction;
	unsigned char colors[351];
};
struct fan_curve {
	int temp;
	int speed;
};

struct port_data {
	int fan_speed, fan_speed_rpm, fan_count;
	struct fan_curve *points;
	int points_used_len;
	int points_total_len;
	struct rgb_data inner_rgb, outer_rgb;
	struct proc_dir_entry *proc_port;
	struct proc_dir_entry *proc_fan_count;
	struct proc_dir_entry *proc_fan_speed;
	struct proc_dir_entry *proc_fan_curve;
	struct proc_dir_entry *proc_inner_and_outer_rgb;
	struct proc_dir_entry *proc_inner_rgb;
	struct proc_dir_entry *proc_outer_rgb;
	struct proc_dir_entry *proc_inner_colors;
	struct proc_dir_entry *proc_outer_colors;
};
static struct port_data ports[4];
static struct rgb_data prev_port_inner_data[4];
static struct rgb_data prev_port_outer_data[4];

static int mb_sync_state;

static struct usb_device *dev;
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_mbsync;

static struct usb_device_id dev_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{},
};
MODULE_DEVICE_TABLE(usb, dev_table);

static int send_rgb_header(int port, int fan_count);
static int send_rgb_colors(int port, int in_or_out, struct rgb_data *colors);
static int send_rgb_mode(int port, int in_or_out, struct rgb_data *data);
void tmp_speed_function(void);
void speed_wq_function(struct work_struct *work);
static struct work_struct speed_wq;

static struct timer_list speed_timer;

void timer_callback_handler(struct timer_list *tl);
int get_fan_speed_from_temp(struct port_data *p, int temp);
static void set_speeds(int port_one, int port_two, int port_three, int port_four);
static void set_speed(int port, int new_speed);
static int get_cpu_temp(void);

void timer_callback_handler(struct timer_list *tl) 
{
	if (!work_pending(&speed_wq)) schedule_work(&speed_wq);
	mod_timer(&speed_timer, jiffies + msecs_to_jiffies(2000));
}

static int prev_temp = 0;
void speed_wq_function(struct work_struct *work)
{
	int new_temp = get_cpu_temp()/1000;
	if (new_temp == -1) {
		printk(KERN_ERR"Lian Li Hub: speed_wq_function failed get_cpu_temp\n");
		return;
	}
	if (prev_temp != new_temp) {
		int new_speed_one   = get_fan_speed_from_temp(&ports[0], new_temp);
		int new_speed_two   = get_fan_speed_from_temp(&ports[1], new_temp);
		int new_speed_three = get_fan_speed_from_temp(&ports[2], new_temp);
		int new_speed_four  = get_fan_speed_from_temp(&ports[3], new_temp);
		set_speeds(new_speed_one, new_speed_two, new_speed_three, new_speed_four);
		prev_temp = new_temp;
	}
}

int get_fan_speed_from_temp(struct port_data *p, int temp) 
{
	struct fan_curve prev_point = { 1, p->points[0].speed };
	for (int i = 0; i < p->points_used_len; i++) {
		int xone = prev_point.temp, xtwo = p->points[i].temp;
		int yone = 100 - prev_point.speed, ytwo = 100 - p->points[i].speed;
		if (xone <= temp && xtwo >= temp) {
			int a = ((ytwo - yone)*10000)/(xtwo - xone);
			int b = yone*10000 - a * xone;
			int tmp = (a * temp + b)/10000;
			return tmp;
		}
		prev_point = p->points[i];
	}
	if (p->points[p->points_used_len-1].temp < temp) {
		return p->points[p->points_used_len-1].speed;
	}
	return 0;
}

int get_cpu_temp(void) 
{
	struct thermal_zone_device *thermal_dev;
	int temp;

	thermal_dev = thermal_zone_get_zone_by_name("x86_pkg_temp");
	if (IS_ERR(thermal_dev)) {
		printk(KERN_ERR"Lian Li Hub: failed thermal_zone_get_zone_by_name\n");
		return -1;
	}

	if (thermal_zone_get_temp(thermal_dev, &temp)) {
		printk(KERN_ERR"Lian Li Hub: failed thermal_zone_get_temp\n");
		return -1;
	}
	return temp;
}

static void set_merge(int mode, int speed, int brightness, int direction, struct rgb_data *colors)
{
	colors->brightness = brightness;
	colors->direction  = direction;
	colors->mode       = mode;
	colors->speed      = speed;
	send_rgb_header(0, ports[0].fan_count);
	send_rgb_header(1, ports[1].fan_count);
	send_rgb_header(2, ports[2].fan_count);
	send_rgb_header(3, ports[3].fan_count);
	send_rgb_colors(0, 0, colors);
	send_rgb_mode(0, 0, colors);
	for (int i = 0; i < 4; i++) {
		ports[i].outer_rgb.mode       = mode;
		ports[i].outer_rgb.speed      = speed;
		ports[i].outer_rgb.direction  = direction;
		ports[i].outer_rgb.brightness = brightness;
	       	memcpy(&ports[i].outer_rgb.colors, colors, PACKET_SIZE-2);
		ports[i].inner_rgb.mode       = mode;
               	ports[i].inner_rgb.speed      = speed;
		ports[i].inner_rgb.direction  = direction;
		ports[i].inner_rgb.brightness = brightness;
		memcpy(&ports[i].inner_rgb.colors, colors, PACKET_SIZE-2);
	}
	/*
	unsigned char (*buffer)[PACKET_SIZE] = kcalloc(PACKET_SIZE * 6, sizeof(unsigned char), GFP_KERNEL);
	buffer[0][0] =       buffer[1][0] =       buffer[2][0] =       buffer[3][0] = 0xe0;
	buffer[0][1] =       buffer[1][1] =       buffer[2][1] =       buffer[3][1] = 0x10;
	buffer[0][2] =       buffer[1][2] =       buffer[2][2] =       buffer[3][2] = 0x60;
	buffer[0][3] = 0x01, buffer[1][3] = 0x02, buffer[2][3] = 0x03, buffer[3][3] = 0x04;
	buffer[0][4] = ports[0].fan_count, buffer[1][4] = ports[1].fan_count;
	buffer[2][4] = ports[2].fan_count, buffer[3][4] = ports[3].fan_count;
	buffer[4][0] = 0xe0, buffer[4][1] = 0x30; 
	memcpy(&buffer[4][2], colors->colors, PACKET_SIZE-2);
	buffer[5][0] = 0xe0, buffer[5][1] = 0x10, buffer[5][2] = mode, buffer[5][3] = speed, buffer[5][4] = direction, buffer[5][5] = brightness;
	for (int i = 0; i < 6; i++) {
		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0x0), 0x09, 0x21, 0x02e0, 0x01, &buffer[i], PACKET_SIZE, 100);
		if (ret != PACKET_SIZE) {
			printk(KERN_ERR "Lian Li Hub: set_rgb failed sending packet %d error %d", i, ret);
		}
	}
	for (int i = 0; i < 4; i++) {
			ports[i].outer_rgb.mode       = mode;
			ports[i].outer_rgb.speed      = speed;
			ports[i].outer_rgb.direction  = direction;
			ports[i].outer_rgb.brightness = brightness;
	        	memcpy(&ports[i].outer_rgb.colors, colors, PACKET_SIZE-2);
			ports[i].inner_rgb.mode       = mode;
                	ports[i].inner_rgb.speed      = speed;
			ports[i].inner_rgb.direction  = direction;
			ports[i].inner_rgb.brightness = brightness;
			memcpy(&ports[i].inner_rgb.colors, colors, PACKET_SIZE-2);

	}
	kfree(buffer);
	*/
}

static void set_all(int flag, int rgb_mode_flag, int mode, int speed, int brightness, int direction, struct rgb_data *outer_colors, struct rgb_data *inner_colors)
{
	// if only_inner || only_outer then send header, colors outer, header, colors inner. per port
	// write previos rgb mode data and colors to outer or inner depenting on which is set. do that from
	// userspace app because you need to save some stuff
	inner_colors->brightness = outer_colors->brightness = brightness;
	inner_colors->direction  = outer_colors->direction  = direction;
	inner_colors->speed      = outer_colors->speed      = speed;
	inner_colors->mode       = outer_colors->mode       = mode;
	struct rgb_data *inner_data = inner_colors;
	struct rgb_data *outer_data = outer_colors;
	if (flag & INNER || flag & OUTER) {
		for (int i = 0; i < 4; i++) {
			inner_data = flag == INNER ? inner_colors : &prev_port_inner_data[i];
			outer_data = flag == OUTER ? outer_colors : &prev_port_outer_data[i];
			send_rgb_header(i, ports[i].fan_count);
			send_rgb_colors(i, 1, outer_data);
			send_rgb_header(i, ports[i].fan_count);
			send_rgb_colors(i, 0, inner_data);
		}
	} else if (flag & INNER_AND_OUTER) {
		for (int i = 0; i < 4; i++) {
			send_rgb_header(i, ports[i].fan_count);
			send_rgb_colors(i, 0, inner_data);
			send_rgb_colors(i, 1, outer_data);
		}
	}
	for (int i = 0; i < 4; i++) {
		send_rgb_mode(i, 1, outer_data);
		send_rgb_mode(i, 0, inner_data);
	}

	for (int i = 0; i < 4; i++) {
		if (flag & INNER_AND_OUTER) {
			ports[i].inner_rgb.mode       = ports[i].outer_rgb.mode       = mode;
			ports[i].inner_rgb.speed      = ports[i].outer_rgb.speed      = speed;
			ports[i].inner_rgb.direction  = ports[i].outer_rgb.direction  = direction;
			ports[i].inner_rgb.brightness = ports[i].outer_rgb.brightness = brightness;
	        	memcpy(&ports[i].outer_rgb.colors, outer_colors->colors, PACKET_SIZE - 2);
			memcpy(&ports[i].inner_rgb.colors, inner_colors->colors, PACKET_SIZE - 2);
		} 
		if (flag & INNER || rgb_mode_flag & INNER) {
			prev_port_inner_data[i].mode       = ports[i].inner_rgb.mode       = mode;
                	prev_port_inner_data[i].speed      = ports[i].inner_rgb.speed      = speed;
			prev_port_inner_data[i].direction  = ports[i].inner_rgb.direction  = direction;
			prev_port_inner_data[i].brightness = ports[i].inner_rgb.brightness = brightness;
			memcpy(&ports[i].inner_rgb.colors, inner_colors->colors, PACKET_SIZE - 2);
	        	memcpy(&prev_port_inner_data[i].colors, inner_colors->colors, PACKET_SIZE - 2);
		} 
		if (flag & OUTER || rgb_mode_flag & OUTER) {
			prev_port_outer_data[i].mode       = ports[i].outer_rgb.mode       = mode;
                        prev_port_outer_data[i].speed      = ports[i].outer_rgb.speed      = speed;
                        prev_port_outer_data[i].direction  = ports[i].outer_rgb.direction  = direction;
                        prev_port_outer_data[i].brightness = ports[i].outer_rgb.brightness = brightness;
	        	memcpy(&ports[i].outer_rgb.colors, outer_colors->colors, PACKET_SIZE - 2);
	        	memcpy(&prev_port_outer_data[i].colors, outer_colors->colors, PACKET_SIZE - 2);
		}
		/* tomorrow save mode if rgb_mode_flag & INNER | OUTER something like that

		if (rgb_mode_flag & INNER) {
			prev_port_inner_data[i].mode       = mode;
                        prev_port_inner_data[i].speed      = speed;
                        prev_port_inner_data[i].direction  = direction;
                        prev_port_inner_data[i].brightness = brightness;
	        	memcpy(&prev_port_inner_data[i].colors, inner_colors->colors, PACKET_SIZE - 2);
		}
		if (rgb_mode_flag & OUTER) {
			prev_port_outer_data[i].mode       = mode;
                        prev_port_outer_data[i].speed      = speed;
                        prev_port_outer_data[i].direction  = direction;
                        prev_port_outer_data[i].brightness = brightness;
	        	memcpy(&prev_port_outer_data[i].colors, outer_colors->colors, PACKET_SIZE - 2);
		}
		*/
	}
        /*const size_t packet_size = 353;
	size_t packets_needed = 16;
	int not_moving = 0;
	int only_inner = 0, only_outer = 0;

	if (mode == 1 || mode == 2) {
	        packets_needed = 20;
	        not_moving = 1;
	}
	if (flag & OUTER) {
		only_outer = 1;
		only_inner = 0;
		packets_needed = 24;
	} else if (flag & INNER) {
		only_outer = 0;
		only_inner = 1;
		packets_needed = 24;
	} else if (flag & INNER_AND_OUTER) {
		not_moving = 1;
		packets_needed = 20;
	}
	unsigned char (*buffer)[353] = kcalloc(packet_size * packets_needed, sizeof(unsigned char), GFP_KERNEL);

	int j = 0;
	int port = 0;
	for (int i = 0; i < packets_needed - 8; i += (2 + not_moving + only_inner + only_outer)) {
	        buffer[i][0] = 0xe0;
	        buffer[i][1] = 0x10;
	        buffer[i][2] = 0x60;
	        buffer[i][3] = port + 1;
	        buffer[i][4] = ports[port].fan_count;
	        buffer[i + 1][0] = 0xe0;
	        buffer[i + 1][1] = 0x30 + only_inner + only_outer + j;
		if (only_outer || only_inner) memcpy(&buffer[i + 1][2], outer_colors->colors, packet_size - 2);
		else memcpy(&buffer[i + 1][2], inner_colors->colors, packet_size - 2);
		if (only_outer || only_inner) {
			buffer[i + 2][0] = 0xe0;
			buffer[i + 2][1] = 0x10;
			buffer[i + 2][2] = 0x60;
			buffer[i + 2][3] = port + 1;
			buffer[i + 2][4] = ports[port].fan_count;
			buffer[i + 3][0] = 0xe0;
			buffer[i + 3][1] = 0x31 - (only_inner + only_outer) + j;
			memcpy(&buffer[i + 3][2], inner_colors->colors, packet_size - 2);
			i += 1;

		} else if (not_moving) {
                	buffer[i + 2][0] = 0xe0;
               		buffer[i + 2][1] = 0x31 + j;
			memcpy(&buffer[i + 2][2], outer_colors->colors, packet_size-2);
		}
		j += 2;
		port = port + 1;
        }
	j = 0;
	for (int i = 0; i < 8; i += 2) {
		buffer[packets_needed-8+i][0] = 0xe0;
		buffer[packets_needed-8+i][1] = 0x11 + j;
		if (flag & OUTER || flag & INNER_AND_OUTER) {
		        buffer[packets_needed-8+i][2] = mode;
		        buffer[packets_needed-8+i][3] = speed;
		        buffer[packets_needed-8+i][4] = direction;
		        buffer[packets_needed-8+i][5] = brightness;
		} else {
	                buffer[packets_needed-8+i][2] = ports[i/2].outer_rgb.mode;
	                buffer[packets_needed-8+i][3] = ports[i/2].outer_rgb.speed;
	                buffer[packets_needed-8+i][4] = ports[i/2].outer_rgb.direction;
	                buffer[packets_needed-8+i][5] = ports[i/2].outer_rgb.brightness;
		}
		buffer[packets_needed-7+i][0] = 0xe0;
		buffer[packets_needed-7+i][1] = 0x10 + j;
		if (flag & INNER || flag & INNER_AND_OUTER) {
		        buffer[packets_needed-7+i][2] = mode;
		        buffer[packets_needed-7+i][3] = speed;
		        buffer[packets_needed-7+i][4] = direction;
		        buffer[packets_needed-7+i][5] = brightness;
		} else {
	                buffer[packets_needed-7+i][2] = ports[i/2].inner_rgb.mode;
	                buffer[packets_needed-7+i][3] = ports[i/2].inner_rgb.speed;
	                buffer[packets_needed-7+i][4] = ports[i/2].inner_rgb.direction;
	                buffer[packets_needed-7+i][5] = ports[i/2].inner_rgb.brightness;
		}
		j += 2;
	}
	for (int i = 0; i < packets_needed; i++) {
		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0x0), 0x09, 0x21, 0x02e0, 0x01, &buffer[i], packet_size, 1000);
		if (ret != packet_size) {
			printk(KERN_ERR "Lian Li Hub: set_rgb failed sending packet %d error %d", i, ret);
		}
	}
	for (int i = 0; i < 4; i++) {
		if (flag & OUTER || flag & INNER_AND_OUTER) {
			ports[i].outer_rgb.mode       = mode;
			ports[i].outer_rgb.speed      = speed;
			ports[i].outer_rgb.direction  = direction;
			ports[i].outer_rgb.brightness = brightness;
	        	memcpy(&ports[i].outer_rgb.colors, outer_colors->colors, packet_size-2);
		}
		
		if (flag & INNER || flag & INNER_AND_OUTER) {
			ports[i].inner_rgb.mode       = mode;
                	ports[i].inner_rgb.speed      = speed;
			ports[i].inner_rgb.direction  = direction;
			ports[i].inner_rgb.brightness = brightness;
			memcpy(&ports[i].inner_rgb.colors, inner_colors->colors, packet_size-2);
		}

	}
	kfree(buffer);
	*/

}
static int send_rgb_header(int port, int fan_count)
{
	int ret;
	unsigned char *buffer = kcalloc(PACKET_SIZE, sizeof(unsigned char), GFP_KERNEL);

	buffer[0] = 0xe0; buffer[1] = 0x60; buffer[2] = 0x10;
	buffer[3] = port; buffer[4] = fan_count;
	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0x0), 0x09, 0x21, 0x02e0, 0x01, buffer, PACKET_SIZE, 100);
	if (ret != PACKET_SIZE) {
		printk(KERN_ERR"Lian Li Hub: send_rgb_header failed ret = %d\n", ret);
		return ret;
	}
	kfree(buffer);
	return 0;
}
static int send_rgb_colors(int port, int in_or_out, struct rgb_data *colors)
{
	int ret;
	unsigned char *buffer = kcalloc(PACKET_SIZE, sizeof(unsigned char), GFP_KERNEL);

	buffer[0] = 0xe0; buffer[1] = 0x30 + in_or_out + (2 * port);
	memcpy(&buffer[2], colors->colors, PACKET_SIZE - 2);
	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0x0), 0x09, 0x21, 0x02e0, 0x01, buffer, PACKET_SIZE, 100);
	if (ret != PACKET_SIZE) {
		printk(KERN_ERR"Lian Li Hub: send_rgb_header failed ret = %d\n", ret);
		return ret;
	}
	kfree(buffer);
	return 0;
}

static int send_rgb_mode(int port, int in_or_out, struct rgb_data *data)
{
	int ret;
	unsigned char *buffer = kcalloc(PACKET_SIZE, sizeof(unsigned char), GFP_KERNEL);

	buffer[0] = 0xe0; buffer[1] = 0x10 + in_or_out + (2 * port);
	buffer[2] = data->mode; buffer[3] = data->speed; buffer[4] = data->direction; buffer[5] = data->brightness;
	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0x0), 0x09, 0x21, 0x02e0, 0x01, buffer, PACKET_SIZE, 100);
	if (ret != PACKET_SIZE) {
		printk(KERN_ERR"Lian Li Hub: send_rgb_header failed ret = %d\n", ret);
		return ret;
	}
	kfree(buffer);
	return 0;
}

/*
 * port from 1 to 4
 * sets inner and outer colors with individual packets
 * 1. header 0x10, 0x60
 * 2. inner  0x30 
 * 3. outer  0x31
 * 4. outer  0x11 
 * 5. inner  0x10 
 */ 
static void set_inner_and_outer_rgb(int port, int mode, int speed, int brightness, int direction, int flags)
{
	ports[port-1].inner_rgb.brightness = ports[port-1].outer_rgb.brightness = brightness;
	ports[port-1].inner_rgb.direction  = ports[port-1].outer_rgb.direction 	= direction;
	ports[port-1].inner_rgb.speed      = ports[port-1].outer_rgb.speed 	= speed;
	ports[port-1].inner_rgb.mode       = ports[port-1].outer_rgb.mode       = mode;

	send_rgb_header(port,         ports[port-1].fan_count);
	send_rgb_colors(port - 1, 0, &ports[port-1].inner_rgb);
	send_rgb_colors(port - 1, 1, &ports[port-1].outer_rgb);
	send_rgb_mode  (port - 1, 1, &ports[port-1].outer_rgb);
	send_rgb_mode  (port - 1, 0, &ports[port-1].inner_rgb);
	/*
	const size_t packet_size = 353;
	int ret, not_moving = 0;
	if (mode == 1 || mode == 2) not_moving = 1;
	unsigned char (*buffer)[353] = kcalloc(packet_size * (10 + not_moving), sizeof(unsigned char), GFP_KERNEL);
	unsigned char data[3][353] =  { { 0xe0, 0x10, 0x60, port, ports[port-1].fan_count, }, 
					{ 0xe0, 0x30 + (2 * (port - 1)), },  
					{ 0xe0, 0x31 + (2 * (port - 1)), },  
					/{ 0xe0, 0x11, ports[0].outer_rgb.mode, ports[0].outer_rgb.speed, ports[0].outer_rgb.direction, ports[0].outer_rgb.brightness },  
					{ 0xe0, 0x10, ports[0].inner_rgb.mode, ports[0].inner_rgb.speed, ports[0].inner_rgb.direction, ports[0].inner_rgb.brightness },  
					{ 0xe0, 0x13, ports[1].outer_rgb.mode, ports[1].outer_rgb.speed, ports[1].outer_rgb.direction, ports[1].outer_rgb.brightness },  
					{ 0xe0, 0x12, ports[1].inner_rgb.mode, ports[1].inner_rgb.speed, ports[1].inner_rgb.direction, ports[1].inner_rgb.brightness },  
					{ 0xe0, 0x15, ports[2].outer_rgb.mode, ports[2].outer_rgb.speed, ports[2].outer_rgb.direction, ports[2].outer_rgb.brightness },  
					{ 0xe0, 0x14, ports[2].inner_rgb.mode, ports[2].inner_rgb.speed, ports[2].inner_rgb.direction, ports[2].inner_rgb.brightness },  
					{ 0xe0, 0x17, ports[3].outer_rgb.mode, ports[3].outer_rgb.speed, ports[3].outer_rgb.direction, ports[3].outer_rgb.brightness },  
					{ 0xe0, 0x16, ports[3].inner_rgb.mode, ports[3].inner_rgb.speed, ports[3].inner_rgb.direction, ports[3].inner_rgb.brightness }
					/
	};
	int j = 1 + not_moving;
	for (int i = 0; i < 4; i++) {
		unsigned char tmp_data[2][353] = { { 0xe0, 0x10 + j - not_moving, ports[i].outer_rgb.mode, ports[i].outer_rgb.speed, ports[i].outer_rgb.direction, ports[i].outer_rgb.brightness },  
						   { 0xe0, 0x10 + j - 1 - not_moving, ports[i].inner_rgb.mode, ports[i].inner_rgb.speed, ports[i].inner_rgb.direction, ports[i].inner_rgb.brightness }, };
		memcpy(&buffer[j+1], &tmp_data[0], packet_size);
		memcpy(&buffer[j + 2], &tmp_data[1], packet_size);
		j += 2;
	}
	memcpy(&buffer[0], data[0], packet_size);
	memcpy(&data[1][2], ports[port-1].inner_rgb.colors, packet_size - 2);
	memcpy(&buffer[1], data[1], packet_size);
	if (not_moving == 1) {
		memcpy(&data[2][2], ports[port-1].outer_rgb.colors, packet_size - 2);
		memcpy(&buffer[2], data[2], packet_size);
	}

	for (int i = 0; i < 10 + not_moving; i++) {
		ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0x0), 0x09, 0x21, 0x02e0, 0x01, &buffer[i], packet_size, 1000);
		if (ret != packet_size) {
			printk(KERN_ERR "Lian Li Hub: set_rgb failed sending packet %d error %d", i, ret);
		}
	}
	*/
}

static void set_inner_rgb(int port, int mode, int speed, int brightness, int direction, int flags)
{
	/*
	struct rgb_data *out_rgb = &ports[port-1].outer_rgb;
	const size_t packet_size = 353;
	unsigned char *buffer = kcalloc(packet_size+1, sizeof(*buffer), GFP_KERNEL);
	unsigned char data[4][353] =  { { 0xe0, 0x10, 0x60, port, ports[port-1].fan_count, }, 
					{ 0xe0, 0x30 + (2 * (port - 1)), },  
					{ 0xe0, 0x11 + (2 * (port - 1)), out_rgb->mode, out_rgb->speed, out_rgb->direction, out_rgb->brightness },  
					{ 0xe0, 0x10 + (2 * (port - 1)), mode, speed, direction, brightness } };
	memcpy(&data[1][2], ports[port-1].inner_rgb.colors, packet_size - 2);
	*/
	prev_port_inner_data[port-1].brightness = ports[port-1].inner_rgb.brightness = brightness;
	prev_port_inner_data[port-1].direction  = ports[port-1].inner_rgb.direction  = direction;
	prev_port_inner_data[port-1].speed      = ports[port-1].inner_rgb.speed      = speed;
	prev_port_inner_data[port-1].mode       = ports[port-1].inner_rgb.mode 	     = mode;

	memcpy(prev_port_inner_data[port-1].colors, ports[port-1].inner_rgb.colors, PACKET_SIZE - 2);

	send_rgb_header(port - 1,     ports[port-1].fan_count);
	send_rgb_colors(port - 1, 0, &ports[port-1].inner_rgb);
	send_rgb_mode  (port - 1, 1, &prev_port_outer_data[port-1]);
	send_rgb_mode  (port - 1, 0, &ports[port-1].inner_rgb);
	
	/*
	int err = 0;
	for (int i = 0; i < 4; i++) {
		memcpy(buffer, data[i], packet_size);
		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0x0), 0x09, 0x21, 0x02e0, 0x01, buffer, packet_size, 1000);
		if (ret != packet_size) {
			printk(KERN_ERR "Lian Li Hub: set_rgb failed sending packet %d error %d", i, ret);
			err = 1;
		}
	}*/
}

static void set_outer_rgb(int port, int mode, int speed, int brightness, int direction, int flags)
{
	/*
	struct rgb_data *in_rgb = &ports[port-1].inner_rgb;
	const size_t packet_size = 353;
	unsigned char *buffer = kcalloc(packet_size, sizeof(*buffer), GFP_KERNEL);
	unsigned char data[4][353] =  { { 0xe0, 0x10, 0x60, port, ports[port-1].fan_count, }, 
					{ 0xe0, 0x31 + (2 * (port - 1)), },  
					{ 0xe0, 0x11 + (2 * (port - 1)), mode, speed, direction, brightness },  
					{ 0xe0, 0x10 + (2 * (port - 1)), in_rgb->mode, in_rgb->speed, in_rgb->direction, in_rgb->brightness } };
	memcpy(&data[1][2], ports[port-1].outer_rgb.colors, packet_size - 2);
	int err = 0;
	for (int i = 0; i < 4; i++) {
		memcpy(buffer, data[i], packet_size);
		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0x0), 0x09, 0x21, 0x02e0, 0x01, buffer, packet_size, 1000);
		if (ret != packet_size) {
			printk(KERN_ERR "Lian Li Hub: set_rgb failed sending packet %d error %d", i, ret);
			err = 1;
		}
	}
	if (err == 0) {
		ports[port-1].inner_rgb.mode 	   = mode;
		ports[port-1].inner_rgb.brightness = brightness;
		ports[port-1].inner_rgb.speed      = speed;
		ports[port-1].inner_rgb.direction  = direction;
	}
	*/
	prev_port_outer_data[port-1].brightness = ports[port-1].outer_rgb.brightness = brightness;
	prev_port_outer_data[port-1].direction  = ports[port-1].outer_rgb.direction  = direction;
	prev_port_outer_data[port-1].speed      = ports[port-1].outer_rgb.speed      = speed;
	prev_port_outer_data[port-1].mode       = ports[port-1].outer_rgb.mode 	     = mode;

	memcpy(prev_port_outer_data[port-1].colors, ports[port-1].outer_rgb.colors, PACKET_SIZE - 2);

	send_rgb_header(port - 1,     ports[port-1].fan_count);
	send_rgb_colors(port - 1, 1, &ports[port-1].outer_rgb);
	send_rgb_mode  (port - 1, 1, &ports[port-1].outer_rgb);
	send_rgb_mode  (port - 1, 0, &prev_port_inner_data[port-1]);
}

static ssize_t read_colors(struct file *f, char *ubuf, size_t count, loff_t *offs)
{
	size_t textsize = 702;
	char *text = kcalloc(textsize, sizeof(*text), GFP_KERNEL);
	int to_copy, not_copied, delta;
	int port = 0;
	const char *parent_name = f->f_path.dentry->d_parent->d_name.name;
	const char *name = f->f_path.dentry->d_name.name;
	unsigned char *colors;

	to_copy = min(count, textsize);
	if (strcmp(parent_name, "Port_one") == 0) {
		port = 0;
	} else if (strcmp(parent_name, "Port_two") == 0) {
		port = 1;
	} else if (strcmp(parent_name, "Port_three") == 0) {
		port = 2;
	} else if (strcmp(parent_name, "Port_four") == 0) {
		port = 3;
	}

	if      (strcmp(name, "inner_colors") == 0) colors = ports[port].inner_rgb.colors;
	else if (strcmp(name, "outer_colors") == 0) colors = ports[port].outer_rgb.colors;

	int text_i = 0;
	for (int i = 0; i < 351; i++) {
		sprintf(&text[text_i], "%02x", colors[i]);
		text_i += 2;
	}

	not_copied = copy_to_user(ubuf, text, to_copy);
	delta = to_copy - not_copied;

	kfree(text);

	if (*offs) return 0;
	else *offs = textsize;

	return delta;
}

static ssize_t write_colors(struct file *f, const char *ubuf, size_t count, loff_t *offs)
{
	size_t text_size = 351;
	char *text = kcalloc(text_size*2, sizeof(*text), GFP_KERNEL);
	int to_copy, not_copied, copied;
	int port = 0;
	const char *parent_name = f->f_path.dentry->d_parent->d_name.name;
	const char *name = f->f_path.dentry->d_name.name;
	unsigned char *colors;

	if (strcmp(parent_name, "Port_one") == 0) {
		port = 0;
	} else if (strcmp(parent_name, "Port_two") == 0) {
		port = 1;
	} else if (strcmp(parent_name, "Port_three") == 0) {
		port = 2;
	} else if (strcmp(parent_name, "Port_four") == 0) {
		port = 3;
	}

	if      (strcmp(name, "inner_colors") == 0) colors = ports[port].inner_rgb.colors;
	else if (strcmp(name, "outer_colors") == 0) colors = ports[port].outer_rgb.colors;

	to_copy = min(count, text_size*2);

	not_copied = copy_from_user(text, ubuf, to_copy);

	copied = to_copy - not_copied;

	int text_i = 0;
	for (int i = 0; i < text_size; i++) {
		unsigned int tmpc;
		sscanf(&text[text_i], "%02x", &tmpc); 
		colors[i] = tmpc;
		text_i += 2;
	}

	kfree(text);
	return copied;

}

static ssize_t read_rgb(struct file *f, char *ubuf, size_t count, loff_t *offs)
{
	size_t textsize = 128;
	char *text = kcalloc(textsize, sizeof(*text), GFP_KERNEL);
	int to_copy, not_copied, delta;
	int port = 0;
	const char *parent_name = f->f_path.dentry->d_parent->d_name.name;

	to_copy = min(count, textsize);
	if (strcmp(parent_name, "Port_one") == 0) {
		port = 0;
	} else if (strcmp(parent_name, "Port_two") == 0) {
		port = 1;
	} else if (strcmp(parent_name, "Port_three") == 0) {
		port = 2;
	} else if (strcmp(parent_name, "Port_four") == 0) {
		port = 3;
	}

	sprintf(text, "%d %d %d %d\n%d %d %d %d\n",
			ports[port].inner_rgb.mode, ports[port].inner_rgb.speed, ports[port].inner_rgb.direction, ports[port].inner_rgb.brightness, 
			ports[port].outer_rgb.mode, ports[port].outer_rgb.speed, ports[port].outer_rgb.direction, ports[port].outer_rgb.brightness);

	not_copied = copy_to_user(ubuf, text, to_copy);
	delta = to_copy - not_copied;

	kfree(text);

	if (*offs) return 0;
	else *offs = textsize;

	return delta;
}

static ssize_t write_rgb(struct file *f, const char *ubuf, size_t count, loff_t *offs)
{
	const char *parent_name = f->f_path.dentry->d_parent->d_name.name;
	const char *name = f->f_path.dentry->d_name.name;
	char *text = kcalloc(64, sizeof(char), GFP_KERNEL);

	int to_copy, not_copied, delta;
	int mode, speed, direction, brightness; 
	int apply_to_all = 0;
	int port = 0;
	void (*func_to_exec)(int p, int m, int s, int b, int d, int f);
	if (strcmp(parent_name, "Port_one") == 0) {
		port = 0;
	} else if (strcmp(parent_name, "Port_two") == 0) {
		port = 1;
	} else if (strcmp(parent_name, "Port_three") == 0) {
		port = 2;
	} else if (strcmp(parent_name, "Port_four") == 0) {
		port = 3;
	}

	to_copy = min(count, 64);
	not_copied = copy_from_user(text, ubuf, to_copy);
	int flags = 0, rgb_mode_flags = 0;
	sscanf(text, "%d %d %d %d %d %d", &mode, &speed, &direction, &brightness, &rgb_mode_flags, &apply_to_all);
	if (strcmp(name, "inner_and_outer_rgb") == 0) { 
		flags = INNER_AND_OUTER;
		func_to_exec = set_inner_and_outer_rgb;
	} else if (strcmp(name, "inner_rgb") == 0) { 
		flags = INNER;
		func_to_exec = set_inner_rgb;
	} else if (strcmp(name, "outer_rgb") == 0) { 
		flags = OUTER;
		func_to_exec = set_outer_rgb;
	}
	if (rgb_mode_flags & MERGE) {
		set_merge(mode, speed, brightness, direction, &ports[port].inner_rgb);
	} else if (apply_to_all) {
		set_all(flags, rgb_mode_flags, mode, speed, brightness, direction, &ports[port].outer_rgb, &ports[port].inner_rgb);
	} else {
		func_to_exec(port+1,  mode, speed, brightness, direction, rgb_mode_flags);
	} 

	delta = to_copy - not_copied;

	kfree(text);
	return delta;
}

static void get_speed_in_rpm(void)
{
	int size = 65;
	unsigned char *response = kcalloc(size, sizeof(*response), GFP_KERNEL);
	int ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0x80), 0x01, 0xa1, 0x01e0, 1, response, size, 1000);
	if (ret != size) {
		printk(KERN_ERR "Lian Li Hub: get_speed failed error %d\n", ret);
	} else {
		ports[0].fan_speed_rpm = (response[2] << 8) + response[3];
		ports[1].fan_speed_rpm = (response[4] << 8) + response[5];
		ports[2].fan_speed_rpm = (response[6] << 8) + response[7];
		ports[3].fan_speed_rpm = (response[8] << 8) + response[9];
	}
	kfree(response);
}

static void set_speeds(int port_one, int port_two, int port_three, int port_four)
{
	int new_speeds[] = { port_one, port_two, port_three, port_four };
	int datalen = 353;
	unsigned char *mbuff = kcalloc(datalen, sizeof(*mbuff), GFP_KERNEL);
	unsigned char data[5][353] =  { { 0xe0, 0x50, }, 
					{ 0xe0, 0x20, 0x00, new_speeds[0], }, 
					{ 0xe0, 0x21, 0x00, new_speeds[1], }, 
					{ 0xe0, 0x22, 0x00, new_speeds[2], }, 
					{ 0xe0, 0x23, 0x00, new_speeds[3], } };
	for (int i = 0; i < 5; i++) {
		memcpy(mbuff, data[i], 353);
		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, mbuff, datalen, 100);
		if (ret != datalen) {
			printk(KERN_ERR "Lian Li Hub: failed to send usb_control_msg with data[%d] error %d", i, ret);
		} else if (i >= 1) {
			ports[i-1].fan_speed = new_speeds[i-1];
		}
	}

	kfree(mbuff);
}

static void set_speed(int port, int new_speed)
{
	unsigned char (*buffer)[PACKET_SIZE] = kcalloc(PACKET_SIZE * 2, sizeof(unsigned char), GFP_KERNEL);
	
	unsigned char data[2][353] =  { { 0xe0, 0x50, }, 
					{ 0xe0, 0x20 +  (port - 1), 0x00, new_speed, }, };
	
	for (int i = 0; i < 2; i++) {
		memcpy(buffer, data[i], PACKET_SIZE);

		int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, buffer, PACKET_SIZE, 100);

		if (ret != PACKET_SIZE) {
			printk(KERN_ERR "Lian Li Hub: failed to send usb_control_msg with data[%d] error %d", i, ret);
		}
	}
	ports[port-1].fan_speed = new_speed;
	kfree(buffer);
}

static void mb_sync(int enable)
{
	unsigned char *buffer = kcalloc(PACKET_SIZE, sizeof(unsigned char), GFP_KERNEL);
	buffer[0] = 0xe0; buffer[1] = 0x10; buffer[2] = 0x61; buffer[3] = enable;

	int ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x09, 0x21, 0x02e0, 1, buffer, PACKET_SIZE, 100);
	if (ret != PACKET_SIZE) {
		printk(KERN_ERR "Lian Li Hub: failed to send mb sync error %d", ret);
		return;
	}
	mb_sync_state = enable;
}

static ssize_t read_mbsync(struct file *f, char *ubuf, size_t count, loff_t *offs)
{
	size_t textsize = 3;
	char *text = kcalloc(textsize, sizeof(*text), GFP_KERNEL);
	int to_copy, not_copied, delta;

	to_copy = min(count, textsize);

	sprintf(text, "%d\n", mb_sync_state);
	not_copied = copy_to_user(ubuf, text, to_copy);
	delta = to_copy - not_copied;

	kfree(text);

	if (*offs) return 0;
	else *offs = textsize;

	return delta;
}

static ssize_t write_mbsync(struct file *f, const char *ubuf, size_t count, loff_t *offs)
{
	size_t textsize = 3;
	char *text = kcalloc(textsize, sizeof(*text), GFP_KERNEL);
	int to_copy, not_copied, delta;
	int sync;

	to_copy = min(count, textsize);

	not_copied = copy_from_user(text, ubuf, to_copy);

	sscanf(text, "%d", &sync);

	mb_sync(sync);

	delta = to_copy - not_copied;

	kfree(text);
	return delta;
}
static ssize_t read_fan_speed(struct file *f, char *ubuf, size_t count, loff_t *offs)
{
	get_speed_in_rpm();
	size_t textsize = 64;
	char *text = kcalloc(textsize, sizeof(*text), GFP_KERNEL);
	int to_copy, not_copied, copied;
	int port = 0;
	const char *parent_name = f->f_path.dentry->d_parent->d_name.name;

	if (strcmp(parent_name, "Port_one") == 0) {
		port = 0;
	} else if (strcmp(parent_name, "Port_two") == 0) {
		port = 1;
	} else if (strcmp(parent_name, "Port_three") == 0) {
		port = 2;
	} else if (strcmp(parent_name, "Port_four") == 0) {
		port = 3;
	}

	sprintf(text, "%d %d", ports[port].fan_speed, ports[port].fan_speed_rpm);

	to_copy = min(count, textsize);

	not_copied = copy_to_user(ubuf, text, to_copy);

	copied = to_copy - not_copied;

	kfree(text);

	//i have no idea what this does but if i dont do it and try to read it doesnt stop printing
	if (*offs) return 0;
	else *offs = textsize;

	return copied;
}

static ssize_t read_fan_curve(struct file *f, char *ubuf, size_t count, loff_t *offs)
{
	const char *parent_name = f->f_path.dentry->d_parent->d_name.name;
	int port = 0;
	if (strcmp(parent_name, "Port_one") == 0) {
		port = 0;
	} else if (strcmp(parent_name, "Port_two") == 0) {
		port = 1;
	} else if (strcmp(parent_name, "Port_three") == 0) {
		port = 2;
	} else if (strcmp(parent_name, "Port_four") == 0) {
		port = 3;
	}
	size_t textsize = ports[port].points_used_len * 8;
	char *text = kcalloc(textsize, sizeof(*text), GFP_KERNEL);
	int to_copy, not_copied, copied;


	int str_i = 0;
	for (int i = 0; i < ports[port].points_used_len; i++) {
		sprintf(&text[str_i], "%03d %03d\n", ports[port].points[i].speed, ports[port].points[i].temp);
		str_i += 8;
	}

	to_copy = min(count, textsize);

	not_copied = copy_to_user(ubuf, text, to_copy);

	copied = to_copy - not_copied;

	kfree(text);

	//i have no idea what this does but if i dont do it and try to read it doesnt stop printing
	if (*offs) return 0;
	else *offs = textsize;

	return copied;
}

static ssize_t write_fan_curve(struct file *f, const char *ubuf, size_t count, loff_t *offs)
{
	size_t text_size = 128;
	char *text= kcalloc(text_size, sizeof(*text), GFP_KERNEL);
	int to_copy, not_copied, copied;
	int port = 0;
	const char *parent_name = f->f_path.dentry->d_parent->d_name.name;

	if (strcmp(parent_name, "Port_one") == 0) {
		port = 0;
	} else if (strcmp(parent_name, "Port_two") == 0) {
		port = 1;
	} else if (strcmp(parent_name, "Port_three") == 0) {
		port = 2;
	} else if (strcmp(parent_name, "Port_four") == 0) {
		port = 3;
	}
	to_copy = min(count, text_size);

	not_copied = copy_from_user(text, ubuf, to_copy);

	copied = to_copy - not_copied;

	int i = 0, str_i = 0, point_i = 0;
	while(text[i] != '\0' && i <= copied) {
		if (text[i] == '\n') {
			if (point_i + 1 > ports[port].points_total_len) {
				//clean up plz
				ports[port].points = krealloc(ports[port].points, sizeof(struct fan_curve) * ports[port].points_total_len + 5, GFP_KERNEL);
				ports[port].points_total_len += 5;
			}
			sscanf(&text[str_i], "%03d %03d\n", &ports[port].points[point_i].speed, &ports[port].points[point_i].temp);
			str_i = i+1;
			point_i += 1;
		}
		i++;
	}
	ports[port].points_used_len = point_i;

	kfree(text);
	return copied;
}

static ssize_t write_fan_count(struct file *f, const char *ubuf, size_t count, loff_t *offs)
{
	size_t text_size = 16;
	char *text = kcalloc(text_size, sizeof(*text), GFP_KERNEL);
	int to_copy, not_copied, copied;
	const char *parent_name = f->f_path.dentry->d_parent->d_name.name;
	int fan_count = 0;

	to_copy = min(count, text_size);
	
	not_copied = copy_from_user(text, ubuf, to_copy);
	
	copied = to_copy - not_copied;

	sscanf(text, "%d", &fan_count);
	
	if (strcmp(parent_name, "Port_one") == 0) {
		ports[0].fan_count = fan_count;
	} else if (strcmp(parent_name, "Port_two") == 0) {
		ports[1].fan_count = fan_count;
	} else if (strcmp(parent_name, "Port_three") == 0) {
		ports[2].fan_count = fan_count;
	} else if (strcmp(parent_name, "Port_four") == 0) {
		ports[3].fan_count = fan_count;
	}

	kfree(text);
	return copied;
}

static ssize_t read_fan_count(struct file *f, char *ubuf, size_t count, loff_t *offs)
{
	size_t text_size = 16;
	char *text = kcalloc(text_size, sizeof(*text), GFP_KERNEL);
	int to_copy, not_copied, copied;
	const char *parent_name = f->f_path.dentry->d_parent->d_name.name;
	int port = 0;

	if (strcmp(parent_name, "Port_one") == 0) {
		port = 0;
	} else if (strcmp(parent_name, "Port_two") == 0) {
		port = 1;
	} else if (strcmp(parent_name, "Port_three") == 0) {
		port = 2;
	} else if (strcmp(parent_name, "Port_four") == 0) {
		port = 3;
	}

	sprintf(text, "%d", ports[port].fan_count);

	to_copy = min(count, text_size);

	not_copied = copy_to_user(ubuf, text, to_copy);

	copied = to_copy - not_copied;

	if (*offs) return 0;
	else *offs = text_size;

	return copied;
}

static struct proc_ops pops_colors = {
	.proc_read = read_colors,
	.proc_write = write_colors,
};

static struct proc_ops pops_fan_count = {
	.proc_read = read_fan_count,
	.proc_write = write_fan_count,
};

static struct proc_ops pops_rgb = {
	.proc_read = read_rgb,
	.proc_write = write_rgb,
};

static struct proc_ops pops_fan_speed = {
	.proc_read = read_fan_speed,
};

static struct proc_ops pops_fan_curve = {
	.proc_read = read_fan_curve,
	.proc_write = write_fan_curve,
};

static struct proc_ops pops_mb_sync = {
	.proc_read = read_mbsync,
	.proc_write = write_mbsync,
};

#define ERROR(str, i) { printk(KERN_ERR "Lian Li Hub: %s, %d\n", str, i); return -1; }
static int dev_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	dev = interface_to_usbdev(intf);
	if (dev == NULL) ERROR("error getting dev from intf", 0);
	
	proc_dir = proc_mkdir("Lian_li_hub", NULL);
	if (proc_dir == NULL) ERROR("proc_mkdir Lian li hub failed", 0);

	proc_mbsync = proc_create("mb_sync", 0666, proc_dir, &pops_mb_sync);
	if (proc_mbsync == NULL) ERROR("proc_create mb_sync failed", 0);
	for (int i = 0; i < 4; i++) {
		switch (i) {
			case 0:
				ports[i].proc_port = proc_mkdir("Port_one", proc_dir);
				if (ports[i].proc_port == NULL) ERROR("proc_mkdir port_one failed", i);
				break;
			case 1:
				ports[i].proc_port = proc_mkdir("Port_two", proc_dir);
				if (ports[i].proc_port == NULL) ERROR("proc_mkdir port_two failed", i);
				break;
			case 2:
				ports[i].proc_port = proc_mkdir("Port_three", proc_dir);
				if (ports[i].proc_port == NULL) ERROR("proc_mkdir port_three failed", i);
				break;
			case 3:
				ports[i].proc_port = proc_mkdir("Port_four", proc_dir);
				if (ports[i].proc_port == NULL) ERROR("proc_mkdir port_four failed", i);
				break;
		}
		ports[i].proc_fan_count = proc_create("fan_count", 0666, ports[i].proc_port, &pops_fan_count);
		if (ports[i].proc_fan_count == NULL) ERROR("proc_create fan_count_one failed", i);
		ports[i].proc_inner_and_outer_rgb = proc_create("inner_and_outer_rgb", 0666, ports[i].proc_port, &pops_rgb);
		if (ports[i].proc_inner_and_outer_rgb == NULL) ERROR("proc_create inner_and_outer_rgb_one failed", i);
		ports[i].proc_inner_rgb = proc_create("inner_rgb", 0666, ports[i].proc_port, &pops_rgb);
		if (ports[i].proc_inner_rgb == NULL) ERROR("proc_create inner_rgb_one failed", i);
		ports[i].proc_outer_rgb = proc_create("outer_rgb", 0666, ports[i].proc_port, &pops_rgb);
		if (ports[i].proc_outer_rgb == NULL) ERROR("proc_create outer_rgb_one failed", i);
		ports[i].proc_inner_colors = proc_create("inner_colors", 0666, ports[i].proc_port, &pops_colors);
		if (ports[i].proc_inner_colors == NULL) ERROR("proc_create inner_colors_one failed", i);
		ports[i].proc_outer_colors = proc_create("outer_colors", 0666, ports[i].proc_port, &pops_colors);
		if (ports[i].proc_outer_colors == NULL) ERROR("proc_create outer_colors_one failed", i);
		ports[i].proc_fan_curve = proc_create("fan_curve", 0666, ports[i].proc_port, &pops_fan_curve);
		if (ports[i].proc_fan_curve == NULL) ERROR("proc_create fan_curve failed", i);
		ports[i].proc_fan_speed = proc_create("fan_speed", 0666, ports[i].proc_port, &pops_fan_speed);
		if (ports[i].proc_fan_speed == NULL) ERROR("proc_create fan_speed failed", i);
	}


	get_speed_in_rpm();

	
	for (int i = 0; i < 4; i++) {
		ports[i].points = kmalloc(sizeof(struct fan_curve) * 8, GFP_KERNEL);
		ports[i].points[0].speed = 85, ports[i].points[0].temp = 15;
		ports[i].points[1].speed = 75, ports[i].points[1].temp = 25;
		ports[i].points[2].speed = 73, ports[i].points[2].temp = 35;
		ports[i].points[3].speed = 68, ports[i].points[3].temp = 42;
		ports[i].points[4].speed = 60, ports[i].points[4].temp = 55;
		ports[i].points[5].speed = 35, ports[i].points[5].temp = 70;
		ports[i].points[6].speed = 10, ports[i].points[6].temp = 80;

		ports[i].points_total_len = 8;
		ports[i].points_used_len = 7;
	}

	INIT_WORK(&speed_wq, speed_wq_function);

 	timer_setup(&speed_timer, timer_callback_handler, 0);
 	mod_timer(&speed_timer, jiffies + msecs_to_jiffies(2000));
	printk(KERN_INFO"Lian Li Hub: driver probed\n");
	return 0;
}

static void dev_disconnect(struct usb_interface *intf)
{
  	del_timer(&speed_timer);
	flush_work(&speed_wq);
	for (int i = 0; i < 4; i++) {
		kfree(ports[i].points);
		proc_remove(ports[i].proc_fan_count);
		proc_remove(ports[i].proc_fan_curve);
		proc_remove(ports[i].proc_inner_and_outer_rgb);
		proc_remove(ports[i].proc_inner_rgb);
		proc_remove(ports[i].proc_inner_colors);
		proc_remove(ports[i].proc_outer_rgb);
		proc_remove(ports[i].proc_outer_colors);
		proc_remove(ports[i].proc_port);
	}
	proc_remove(proc_dir);
	printk(KERN_INFO "Lian Li Hub: driver disconnect\n");
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
	printk(KERN_INFO "Lian Li Hub: driver init\n");
	res = usb_register(&driver);
	if (res) {
		printk(KERN_ERR "Lian Li Hub: Error during register\n");
		return -res;
	}
	return 0;
}

static void __exit controller_exit(void)
{
	usb_deregister(&driver);
	printk(KERN_INFO "Lian Li Hub: driver exit\n");
}

module_init(controller_init);
module_exit(controller_exit);
