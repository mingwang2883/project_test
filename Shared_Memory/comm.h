#ifndef _COMM_H__
#define _COMM_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define SHARED_MEMORY_ID        0x6666
#define SHARED_MEMORY_NAME      "./sd_check"

#define MAX_DATA_BUFFER         256

int CreateShm(char *path,int size);
int DestroyShm(int shmid);
int GetShm(char *path,int size);

#endif 
