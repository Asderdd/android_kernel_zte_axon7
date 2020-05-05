#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/input/mt.h>
#include "coordinator.h"

#define HANDLE_DELAY_MS	500
#define MAX_FINGER			20
#define MAX_FINGER_MIRROR	10
#define SINGLR_MAX_X		1080
#define SINGLR_MAX_Y		1920
#define MAJOR_PANEL		0
#define MINOR_PANEL		1
#define AIM_FREQUENCY		140

#define TEB_INFO(fmt, arg...)           pr_info("<<TEB-INF>>[%s:%d] "fmt"", __func__, __LINE__, ##arg)
#define TEB_ERROR(fmt, arg...)          pr_info("<<TEB-ERR>>[%s:%d] "fmt"", __func__, __LINE__, ##arg)

typedef struct {
	unsigned int wx;
	unsigned int wy;
	int x;
	int y;
	unsigned char slot_id;
	unsigned char panel_id;
	unsigned char tool_finger;
	struct timeval tv;
	struct list_head list;
} COORD_INFO_T;

typedef struct {
	struct delayed_work work;
	struct workqueue_struct *workqueue;
	struct list_head touch_list_head[MAX_FINGER];
	struct mutex list_mutex;
	struct input_dev *input_dev;
	unsigned int wait_flag;
	unsigned int finger_flag;
} TOUCH_EXPAND_T;


static TOUCH_EXPAND_T global_touch_expand;
extern bool joystick_mode;
static bool pre_mode = 0;

static DECLARE_WAIT_QUEUE_HEAD(touch_expand_waitq);

void zte_touch_expand_push(int x, int y,
								unsigned int wx, unsigned int wy,
								unsigned char slot_id, unsigned char tool_finger,
								unsigned char panel_id)
{
	struct coordinate position[1];
	COORD_INFO_T *fragment_info = NULL;
	unsigned char list_id = 0;
	COORD_INFO_T *fragment_info_mirror = NULL;
	unsigned char list_id_mirror = 0;
	int touch_flag = 0;

	fragment_info = kzalloc(sizeof(COORD_INFO_T), GFP_KERNEL);
	if (fragment_info == NULL) {
		TEB_ERROR("falied to kmalloc fragment info\n");
		return;
	}

	fragment_info_mirror = kzalloc(sizeof(COORD_INFO_T), GFP_KERNEL);
	if (fragment_info_mirror == NULL) {
		TEB_ERROR("falied to kmalloc fragment_info_mirror\n");
		kfree(fragment_info);
		return;
	}

	if (joystick_mode)
		pre_mode = 1;

	fragment_info->y = y;
	fragment_info->wx = wx;
	fragment_info->wy = wy;
	fragment_info->panel_id = panel_id;
	fragment_info->tool_finger = tool_finger;

	fragment_info_mirror->wx = wx;
	fragment_info_mirror->wy = wy;
	fragment_info_mirror->panel_id = panel_id ? 0 : 1;
	fragment_info_mirror->tool_finger = tool_finger;

	if (panel_id == MINOR_PANEL) {
		fragment_info->x = x + SINGLR_MAX_X;
		fragment_info->slot_id = slot_id + MAX_FINGER/2;
		list_id = slot_id + MAX_FINGER/2;
	} else {
		fragment_info->x = x;
		fragment_info->slot_id = slot_id;
		list_id = slot_id;

		touch_flag = global_touch_expand.finger_flag & (1 << fragment_info->slot_id);
		if (!touch_flag && tool_finger)
			zte_down(slot_id, x, y);

		position[0].x = x;
		position[0].y = y;
		zte_get_offset(slot_id, position);
		if (!tool_finger)
			zte_up(slot_id);

		fragment_info_mirror->x = position[0].x;
		fragment_info_mirror->y = position[0].y;

		if (slot_id > (MAX_FINGER_MIRROR / 2 - 1))
			fragment_info_mirror->slot_id = slot_id + (MAX_FINGER / 2);
		else
			fragment_info_mirror->slot_id = slot_id +
					(MAX_FINGER / 2) + (MAX_FINGER_MIRROR / 2);
		list_id_mirror = fragment_info_mirror->slot_id;
	}

	/*do_gettimeofday(&fragment_info->tv);*/

	/*TEB_INFO("+++++ x %u, y %u, slot_id %u, panel_id %u!!!\n",
				fragment_info->x, fragment_info->y, fragment_info->slot_id, fragment_info->panel_id);*/

	/*TEB_INFO("lock + lock + lock + lock\n");*/
	mutex_lock(&global_touch_expand.list_mutex);
	list_add(&(fragment_info->list), &global_touch_expand.touch_list_head[list_id]);
	if ((joystick_mode || pre_mode) && (panel_id == MAJOR_PANEL))
		list_add(&(fragment_info_mirror->list),
			&global_touch_expand.touch_list_head[list_id_mirror]);
	global_touch_expand.wait_flag = true;
	mutex_unlock(&global_touch_expand.list_mutex);
	/*TEB_INFO("unlock - unlock - unlock - unlock\n");*/

	wake_up_interruptible(&touch_expand_waitq);
}
EXPORT_SYMBOL(zte_touch_expand_push);


static void zte_touch_expand_handle_fragment(COORD_INFO_T *report_info[])
{
	struct input_dev *input_dev = global_touch_expand.input_dev;
	COORD_INFO_T *fragment_info = NULL;
	unsigned char finger = 0;
	unsigned long delay_time = 0;
	/*struct timeval after_tv;*/
	/*truct timeval *before_tv = NULL;*/
	/*unsigned int elapsed_time = 0;*/
	static unsigned short debug_x = 0;
	static unsigned short debug_y = 0;
	static unsigned char debug_max_finger = 0;
	unsigned char temp_max_finger = 0;
	/*unsigned char i = 0;*/

	for (finger = 0; finger < MAX_FINGER; finger++) {

		if (report_info[finger] == NULL)
			continue;

		/*TEB_INFO("-----finger %u, fragment_info %p!!!\n", finger, report_info[finger]);*/

		fragment_info = report_info[finger];
/*
		do_gettimeofday(&after_tv);
		before_tv = &(fragment_info->tv);

		elapsed_time = (after_tv.tv_sec * 1000) + (after_tv.tv_usec / 1000)
					    - ((before_tv->tv_sec * 1000) + (before_tv->tv_usec / 1000));
*/
		/*TEB_INFO("----- elapsed time %u ms,x %u, y %u, slot_id %u, panel_id %u!!!\n",
		elapsed_time, fragment_info->x, fragment_info->y, fragment_info->slot_id, fragment_info->panel_id);*/

		if (fragment_info->tool_finger) {
			input_mt_slot(input_dev, fragment_info->slot_id);
			input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, 1);

			input_report_key(input_dev, BTN_TOUCH, 1);
			input_report_key(input_dev, BTN_TOOL_FINGER, 1);
			input_report_abs(input_dev, ABS_MT_POSITION_X, fragment_info->x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y, fragment_info->y);

			input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
							max(fragment_info->wx, fragment_info->wy));
			input_report_abs(input_dev, ABS_MT_TOUCH_MINOR,
							min(fragment_info->wx, fragment_info->wy));

			global_touch_expand.finger_flag |= (1 << fragment_info->slot_id);

			if ((debug_max_finger == 0) && (temp_max_finger == 0)) {/*first finger touch*/
				debug_x = fragment_info->x;
				debug_y = fragment_info->y;
				TEB_INFO("touch down\n");
			}

			temp_max_finger++;/*max finger in a certain irq*/
/*
			TEB_INFO("++++++++++ %d ", fragment_info->slot_id);
			for (i = 0; i < MAX_FINGER; i++) {
				printk("%d", (global_touch_expand.finger_flag & (1 << i)) ? 1 : 0);
			}
			printk("\n");
*/
		} else if (global_touch_expand.finger_flag & (1 << fragment_info->slot_id)) {
			input_mt_slot(input_dev, fragment_info->slot_id);
			input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, 0);

			global_touch_expand.finger_flag &= ~(1 << fragment_info->slot_id);
/*
			TEB_INFO("----------- %d ", fragment_info->slot_id);
			for (i = 0; i < MAX_FINGER; i++) {
				printk("%d", (global_touch_expand.finger_flag & (1 << i)) ? 1 : 0);
			}
			printk("\n");
*/
		}

		kfree(fragment_info);
	}

	if (temp_max_finger > debug_max_finger)
		debug_max_finger = temp_max_finger;/*max finger in one touch*/

	if (((global_touch_expand.finger_flag & 0x000FFFFF) == 0)
		&& (debug_max_finger)) {
		input_report_key(input_dev, BTN_TOUCH, 0);
		input_report_key(input_dev, BTN_TOOL_FINGER, 0);
		TEB_INFO("touch up, finger: %u, position: [%u, %u]\n", debug_max_finger, debug_x, debug_y);
		debug_max_finger = 0;
		debug_x = 0;
		debug_y = 0;
		if (!joystick_mode)
			pre_mode = 0;
	}

	input_sync(input_dev);

	delay_time = 1000 * 1000 / AIM_FREQUENCY; /*delay us = 1/frq * 1000ms * 1000us */
	usleep_range(delay_time, delay_time + 200);
}

static void zte_touch_expand_handle_work(struct work_struct *work)
{
	COORD_INFO_T *report_info[MAX_FINGER] = {NULL};
	COORD_INFO_T *fragment_info = NULL;
	int retval = 0;
	unsigned char i = 0, found_finger = false;

	while (true) {

		/*TEB_INFO("lock + lock + lock + lock\n");*/

		mutex_lock(&global_touch_expand.list_mutex);

		found_finger = false;

		for (i = 0; i < MAX_FINGER; i++) {
			if (!list_empty(&global_touch_expand.touch_list_head[i])) {
				fragment_info = list_last_entry(&global_touch_expand.touch_list_head[i],
												COORD_INFO_T, list);

				list_del(&(fragment_info->list));
				report_info[i] = fragment_info;
				/*TEB_INFO("+++++finger %u, fragment_info %p!!!\n", i, fragment_info);*/
				found_finger = true;
			} else {
				report_info[i] = NULL;
			}
		}

		if (!found_finger) {
			global_touch_expand.wait_flag = 0;
		}

		mutex_unlock(&global_touch_expand.list_mutex);

		/*TEB_INFO("unlock - unlock - unlock - unlock\n");*/

		if (found_finger) {
			zte_touch_expand_handle_fragment(report_info);
			continue;
		}

		/*TEB_INFO("####### wait -------------!!!\n");*/
		retval = wait_event_interruptible(touch_expand_waitq,
					global_touch_expand.wait_flag);

		if (retval) {
			TEB_INFO("interruptible found,  wait event failed!!!\n");
		}
	}
}

static int zte_touch_expand_init(void)
{
	int retval = 0;
	struct input_dev *input_dev = NULL;
	unsigned char i = 0;

	TEB_INFO("into!!!\n");

	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		TEB_ERROR("Failed to allocate input device\n");
		retval = -ENOMEM;
		goto ExitLoop;
	}

	input_dev->name = "zte-touchsrceen-3nd";
	input_dev->phys = "zte_touch_input";
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0001;
	input_set_drvdata(input_dev, &global_touch_expand);

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);

	input_set_abs_params(input_dev,
			ABS_MT_POSITION_X, 0,
			SINGLR_MAX_X * 2, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_POSITION_Y, 0,
			SINGLR_MAX_Y, 0, 0);

	input_set_abs_params(input_dev,
			ABS_MT_TOUCH_MAJOR, 0,
			255, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_TOUCH_MINOR, 0,
			255, 0, 0);

#if (KERNEL_VERSION(3, 7, 0) <= LINUX_VERSION_CODE)
	input_mt_init_slots(input_dev,
			MAX_FINGER, INPUT_MT_DIRECT);
#else
	input_mt_init_slots(input_dev,
			MAX_FINGER);
#endif

	retval = input_register_device(input_dev);
	if (retval) {
		TEB_ERROR("Failed to register input device\n");
		goto ExitLoop;
	}

	global_touch_expand.input_dev = input_dev;

	mutex_init(&global_touch_expand.list_mutex);

	for (i = 0; i < MAX_FINGER; i++) {
		INIT_LIST_HEAD(&global_touch_expand.touch_list_head[i]);
	}

	global_touch_expand.workqueue = create_singlethread_workqueue("zte_touch_exp_wq");
	INIT_DELAYED_WORK(&global_touch_expand.work, zte_touch_expand_handle_work);

	queue_delayed_work(global_touch_expand.workqueue,
			&global_touch_expand.work,
			msecs_to_jiffies(HANDLE_DELAY_MS));
ExitLoop:
	TEB_INFO("exit!!!\n");

	return 0;
}

static void zte_touch_expand_remove(void)
{
}


module_init(zte_touch_expand_init);
module_exit(zte_touch_expand_remove);
