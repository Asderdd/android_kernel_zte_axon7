#include "coordinator.h"
#include <linux/kernel.h>

#define CONTROL_NUM 22

static int map[10] = {0};
extern struct area areas[CONTROL_NUM];

int zte_in_circle(int x, int y, int center_x, int center_y, int radius)
{
	int distance_x = abs(x - center_x);
	int distance_y = abs(y - center_y);
	int distance_z = int_sqrt(distance_x * distance_x + distance_y * distance_y);

	if (distance_z < (radius / 2))
		return 1;

	return 0;
}

int zte_in_rectangle(int x, int y, int area_x, int area_y, int width, int height)
{
	if ((x >= area_x) && (x <= area_x + width) && (y >= area_y) && (y <= area_y + height))
		return 1;

	return 0;
}

int zte_down(int point_id, int x, int y)
{
	int index = 0;
	int i;
	int temp;

	temp = x;
	x = y;
	y = 1080 - temp;

	pr_notice("%s: x=%d,y=%d,point_id=%d\n", __func__, x, y, point_id);

	for (i = 1; i < CONTROL_NUM; ++i) {
		if (areas[i].shape == 0 && zte_in_circle(x, y, areas[i].center_x, areas[i].center_y, areas[i].width)) {
			index = i;
			break;
		} else if (areas[i].shape == 1 &&
			zte_in_rectangle(x, y, areas[i].x, areas[i].y, areas[i].width, areas[i].height)) {
			index = i;
			break;
		}
	}

	map[point_id] = index;
	return index;
}

void zte_get_offset(int point_id, struct coordinate *position)
{
	if (position == NULL)
		return;

	if ((point_id > -1) && (point_id <= 10)) {
		int index = map[point_id];

		position->x += 1080 - areas[index].delta_y;
		position->y += areas[index].delta_x;
	}
}

int zte_up(int point_id)
{
	int ret = -1;

	if ((point_id > -1) && (point_id <= 10)) {
		map[point_id] = 0;
		ret = 0;
	}

	return ret;
}
