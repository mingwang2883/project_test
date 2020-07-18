#ifndef JK_PUSH_MESSAGE_H
#define JK_PUSH_MESSAGE_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

typedef struct push_message_info
{
    int function_index_;
    int event_type_;
    unsigned long event_happend_time_;
    int feed_weight_;
    char pic_name_[56];
    char alarm_alise_[56];
}struct_push_message_info;

typedef struct push_server_info
{
    char   ip_addrs[24];         //ipaddr
    unsigned int   ip_port;      //port
    char   server_push_path[20]; //server push addr
}struct_pushserver;

int SendRegister();

#endif /* JK_PUSH_MESSAGE_H */
