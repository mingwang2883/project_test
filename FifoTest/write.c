#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE 128

int main()
{
    char buffer[SIZE];
    int write_fd;
    int ret;
    int len;
    int i;
    int j = 0;

    write_fd = open("./swap",O_WRONLY);//打开管道（只写）
    if(write_fd < 0)
    {
        perror("open error");
        exit(1);

    }
    printf("input data\n");

    while(1)
    {
        memset(buffer,0,sizeof(buffer));
        j++;
        for(i = 0;i < j;i++)
        {
            buffer[i] = 'a' + i;
        }
        len = strlen(buffer);
        ret = write(write_fd,buffer,SIZE);
        if(ret == -1)
        {
            perror("write error");
            exit(1);
        }
        printf("the data is : %s\n",buffer);

        if(strcmp(buffer,"exit") == 0)
        {
            break;
        }

        if(j == 26)
        {
            j = 0;
        }
        sleep(1);
    }

    close(write_fd);

    return 0;
}
