
#ifndef _ZTE_TOUCH_PINCTRL_H
#define _ZTE_TOUCH_PINCTRL_H


typedef struct zte_pinctrl_info {
	struct device *dev;
	struct pinctrl *ts_pinctrl;
	char *int_active_describe;
	char *int_suspend_describe;
	char *reset_active_describe;
	char *reset_suspend_describe;
	struct pinctrl_state *ts_state_int_active;
	struct pinctrl_state *ts_state_int_suspend;
	struct pinctrl_state *ts_state_reset_active;
	struct pinctrl_state *ts_state_reset_suspend;
} ZTE_PINCTRL_INFO_T;

int zte_ts_pinctrl_init(ZTE_PINCTRL_INFO_T *pinctrl_node);
int zte_ts_pinctrl_configure(ZTE_PINCTRL_INFO_T *pinctrl_node,
			     struct pinctrl_state *set_state);
int zte_ts_pinctrl_ralease(ZTE_PINCTRL_INFO_T *pinctrl_node);

#endif

