#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void Hex2Str(const unsigned char *sSrc,  char *sDest, int nSrcLen)
{
    int  i;
    char szTmp[3];
    for( i = 0; i < nSrcLen; i++ )
    {
        sprintf( szTmp, "%02X", (unsigned char) sSrc[i] );
        memcpy( &sDest[i * 2], szTmp, 2 );
    }
}


int main()
{
    //unsigned char buf[32] = 0x7F;
    char a[32] = {0};
  //int a = 5;
    int b = 0;

    //Hex2Str(buf,a,sizeof(buf));
    int fd;
    char buf[4] = {0};
    fd = open("./a",O_RDONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    
    read(fd,buf,sizeof(buf));

    b = atoi(buf);

    printf("%d\n",b);
    printf("%02d\n",2);
	printf("Hello world\n");
	return 0;
}
