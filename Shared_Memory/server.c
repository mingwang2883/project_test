#include "comm.h"

int main()
{
    int shmid = CreateShm(SHARED_MEMORY_NAME,MAX_DATA_BUFFER);

    unsigned char *data = shmat(shmid,NULL,0);

    sleep(1);

    while(1)
    {
        if(*data != 0)
        {
            printf("data %d\n",*data);
        }

        sleep(1);
    }

    shmdt(data);
    sleep(2);
    DestroyShm(shmid);

    return 0;
} 
