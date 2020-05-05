
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include "zte_touchscreen_pinctrl.h"

int zte_ts_pinctrl_init(ZTE_PINCTRL_INFO_T *pinctrl_node)
{

	pinctrl_node->ts_pinctrl = devm_pinctrl_get(pinctrl_node->dev);

	if (IS_ERR_OR_NULL(pinctrl_node->ts_pinctrl)) {
		dev_err(pinctrl_node->dev,
			"%s:%dGetting pinctrl handle failed\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	if (pinctrl_node->int_active_describe) {
		dev_info(pinctrl_node->dev, "active describe %s\n", pinctrl_node->int_active_describe);
		pinctrl_node->ts_state_int_active = pinctrl_lookup_state(pinctrl_node->ts_pinctrl,
						pinctrl_node->int_active_describe);
		if (IS_ERR(pinctrl_node->ts_state_int_active)) {
			dev_err(pinctrl_node->dev,
				"cannot get ts pinctrl active state\n");
			return PTR_ERR(pinctrl_node->ts_state_int_active);
		}
	}

	if (pinctrl_node->int_suspend_describe) {
		dev_info(pinctrl_node->dev, "suspend describe %s\n", pinctrl_node->int_suspend_describe);
		pinctrl_node->ts_state_int_suspend = pinctrl_lookup_state(pinctrl_node->ts_pinctrl,
						 pinctrl_node->int_suspend_describe);
		if (IS_ERR(pinctrl_node->ts_state_int_suspend)) {
			dev_err(pinctrl_node->dev,
				"cannot get ts pinctrl suspend state\n");
			return PTR_ERR(pinctrl_node->ts_state_int_suspend);
		}
	}

	if (pinctrl_node->reset_active_describe) {
		dev_info(pinctrl_node->dev, "release describe %s\n", pinctrl_node->reset_active_describe);
		pinctrl_node->ts_state_reset_active = pinctrl_lookup_state(pinctrl_node->ts_pinctrl,
						 pinctrl_node->reset_active_describe);
		if (IS_ERR(pinctrl_node->ts_state_reset_active)) {
			dev_err(pinctrl_node->dev,
				"cannot get ts pinctrl release state\n");
			return PTR_ERR(pinctrl_node->ts_state_reset_active);
		}
	}

	if (pinctrl_node->reset_suspend_describe) {
		dev_info(pinctrl_node->dev, "release describe %s\n", pinctrl_node->reset_suspend_describe);
		pinctrl_node->ts_state_reset_suspend = pinctrl_lookup_state(pinctrl_node->ts_pinctrl,
						 pinctrl_node->reset_suspend_describe);
		if (IS_ERR(pinctrl_node->ts_state_reset_suspend)) {
			dev_err(pinctrl_node->dev,
				"cannot get ts pinctrl release state\n");
			return PTR_ERR(pinctrl_node->ts_state_reset_suspend);
		}
	}

	dev_info(pinctrl_node->dev, "pinctrl init finished\n");

	return 0;
}
EXPORT_SYMBOL(zte_ts_pinctrl_init);

int zte_ts_pinctrl_configure(ZTE_PINCTRL_INFO_T *pinctrl_node,
			     struct pinctrl_state *set_state)
{
	int retval;

	if (IS_ERR_OR_NULL(pinctrl_node->ts_pinctrl)) {
		dev_err(pinctrl_node->dev,
			"%s:%dGetting pinctrl handle failed\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	if (set_state == NULL) {
		dev_err(pinctrl_node->dev,
			"%s:%d pinctrl handle is null\n",
			__func__, __LINE__);
		return -EINVAL;
	}


	retval = pinctrl_select_state(pinctrl_node->ts_pinctrl, set_state);
	if (retval) {
		dev_err(pinctrl_node->dev,
			"cannot set ts pinctrl active state\n");
		return retval;
	}

	return 0;
}
EXPORT_SYMBOL(zte_ts_pinctrl_configure);

int zte_ts_pinctrl_ralease(ZTE_PINCTRL_INFO_T *pinctrl_node)
{

	if (IS_ERR_OR_NULL(pinctrl_node->ts_pinctrl) || (pinctrl_node->dev == NULL)) {
		dev_err(pinctrl_node->dev,
			"%s:%dGetting pinctrl handle failed\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	devm_pinctrl_put(pinctrl_node->ts_pinctrl);

	dev_info(pinctrl_node->dev, "pinctrl ralease\n");

	return 0;
}
EXPORT_SYMBOL(zte_ts_pinctrl_ralease);
