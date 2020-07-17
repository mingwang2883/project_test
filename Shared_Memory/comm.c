#include "comm.h"

static int CommShm(char *path,int size,int flags)
{
    key_t key = ftok(path,PROJ_ID);
    if(key < 0)
    {
        perror("ftok");
        return -1;

    }
    int shmid = 0;
    if((shmid = shmget(key,size,flags)) < 0)
    {
        perror("shmget");
        return -2;

    }
    return shmid;

}

int DestroyShm(int shmid)
{
    if(shmctl(shmid,IPC_RMID,NULL) < 0)
    {
        perror("shmctl");
        return -1;

    }
    return 0;

}

int CreateShm(char *path,int size)
{
    remove(path);
    mkdir(path,S_IRWXU);
    return CommShm(path,size,IPC_CREAT | IPC_EXCL | 0666);
}

int GetShm(char *path,int size)
{
    return CommShm(path,size,IPC_CREAT);
}
