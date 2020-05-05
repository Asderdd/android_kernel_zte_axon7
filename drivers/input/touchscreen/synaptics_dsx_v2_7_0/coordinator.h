#ifndef SGAME_COORDINATOR_H
#define SGAME_COORDINATOR_H

struct coordinate {
	int x;
	int y;
};

struct area {
	int x;
	int y;
	int center_x;
	int center_y;
	int width;
	int height;
	int delta_x;
	int delta_y;
	int shape;
	int reserved;
};

/*
point_id:used for identifying finger
x:the coordinate of X
y:the coordinate of Y
*/
int zte_down(int point_id, int x, int y);

/* Getting the offset attached to point_id */
void zte_get_offset(int point_id, struct coordinate *position);

/* Releasing the offset attached to point_id */
int zte_up(int point_id);
#endif
