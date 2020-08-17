#include "comm.h"

int main()
{
    int i = 0;

    int shmid = GetShm(SHARED_MEMORY_NAME,MAX_DATA_BUFFER);
    unsigned char *data = shmat(shmid,NULL,0);

    sleep(1);

    while(1)
    {
        i++;
        *data = i;
        printf("client# %d\n",*data);

        sleep(1);
    }

    shmdt(data);
    sleep(2);
    return 0;
}
