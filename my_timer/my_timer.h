#ifndef MY_TIMER_H
#define MY_TIMER_H

#include <stdio.h>
#include <time.h>

#define SET_NEW_TIME    1
#define NOT_SET_TIME     0

typedef struct
{
    unsigned int now_time;
    unsigned int last_time;
}struct_my_timer;


#endif // MY_TIMER_H
