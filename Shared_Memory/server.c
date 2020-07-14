#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "comm.h"

int main()
{
    mkdir(PATHNAME,S_IRWXU);

    int shmid = CreateShm(256);

    char *addr = shmat(shmid,NULL,0);
    sleep(2);

    int i = 0;
    while(i++ < 26)
    {
        printf("client# %s\n",addr);
        sleep(1);
    }

    shmdt(addr);
    sleep(2);
    DestroyShm(shmid);

    return 0;
} 
