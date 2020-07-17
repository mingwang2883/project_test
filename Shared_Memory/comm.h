#ifndef _COMM_H__
#define _COMM_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define PATHNAME "./sd_check"
#define PROJ_ID 0x6666

int CreateShm(char *path,int size);
int DestroyShm(int shmid);
int GetShm(char *path,int size);

#endif 
