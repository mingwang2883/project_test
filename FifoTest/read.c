#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SIZE 128

int main()
{
    int max_fd;
    int read_fd;
    int result;
    int nread;
    char buffer[SIZE];

    fd_set inputs;
    struct timeval timeout;

    read_fd = open("./swap",O_RDONLY);
    if(read_fd < 0)
    {
        perror("open error");
        exit(1);
    }

    max_fd = read_fd + 1;

    while(1)
    {
        FD_ZERO(&inputs);//用select函数之前先把集合清零
        FD_SET(read_fd,&inputs);//把要检测的句柄——标准输入（0），加入到集合里。

        timeout.tv_sec = 0;
        timeout.tv_usec = 500 * 1000;
        result = select(max_fd, &inputs,NULL,NULL, &timeout);
        if(result < 0)
        {
            perror("select");
            exit(1);
        }
        else if(result == 0)
        {
//            printf("timeout\n");
        }
        else
        {
            if(FD_ISSET(read_fd,&inputs))
            {
                memset(buffer,0,sizeof(buffer));
                nread = read(read_fd,buffer,SIZE);
                printf("the data is : %s\n",buffer);

                if(strcmp(buffer,"exit") == 0)
                {
                    break;
                }
            }
        }
    }

    close(read_fd);
    return 0;
}

