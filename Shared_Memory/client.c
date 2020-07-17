#include "comm.h"

int main()
{
    int shmid = GetShm(PATHNAME,256);
    sleep(1);

    unsigned char *addr = shmat(shmid,NULL,0);
    sleep(2);
    int i = 0;

    while(1)
    {
        printf("*addr : %d\n",*addr);
        *addr = i;
        sleep(1);
        i++;
    }

    shmdt(addr);
    sleep(2);
    return 0;
}
