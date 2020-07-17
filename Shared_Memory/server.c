#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "comm.h"

int main()
{
    int shmid = CreateShm(PATHNAME,256);

    unsigned char *addr = shmat(shmid,NULL,0);
    sleep(2);

    int i = 0;
    while(1)
    {
        printf("client# %d\n",*addr);
        if(*addr != 0)
        {
            *addr = 0;
        }
        sleep(1);
    }

    shmdt(addr);
    sleep(2);
    DestroyShm(shmid);

    return 0;
} 
