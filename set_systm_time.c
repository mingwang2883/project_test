#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

typedef struct
{
    int year;
    int mon;
    int day;
    int hour;
    int min;
    int sec; 
}struct_date;

int main()
{
    int fd = -1;
    char date_str[20] = {0};
    char buf[20] = {0};
    time_t currTime;
    struct tm mk_time;
    struct tm *currDate = NULL;
    time_t save_time;
    struct timeval set_time;
    struct_date save_date;

    memset(&mk_time,0,sizeof(mk_time));
    time(&currTime);
    currDate = localtime(&currTime);
    memset(date_str, 0, sizeof(date_str));
    strftime(date_str, sizeof(date_str),"%Y-%m-%d %H:%M:%S", currDate);

    //fd = open("./date_save_file.txt",O_CREAT|O_TRUNC|O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    fd = open("/mnt/config/save_last_date.txt",O_RDONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

    //write(fd,date_str,sizeof(date_str));
    //lseek(fd,0,SEEK_SET);
    read(fd,buf,sizeof(buf));

    printf("buf : %s\n",buf);

    sscanf(buf,"%d-%d-%d %d:%d:%d",&save_date.year,&save_date.mon,&save_date.day,&save_date.hour,&save_date.min,&save_date.sec);

    mk_time.tm_year  = save_date.year - 1900;
    mk_time.tm_mon   = save_date.mon - 1;
    mk_time.tm_mday  = save_date.day;
    mk_time.tm_hour  = save_date.hour;
    mk_time.tm_min   = save_date.min;
    mk_time.tm_sec   = save_date.sec;

    save_time = mktime(&mk_time);
    printf("save_time = %ld\n",save_time);

    set_time.tv_sec = save_time;
    settimeofday(&set_time,NULL);
    close(fd);
    return 0;
}

