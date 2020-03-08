#include "my_timer.h"

int second_timer(struct_my_timer *timer,int reset)
{
    time_t now;
    time(&now);

    if (reset == 1)
        timer->last_time = (int)now;

    timer->now_time = (int)now;

    int cur_time;
    cur_time = (int)(timer->now_time - timer->last_time);
    return cur_time;
}
