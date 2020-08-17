#include <stdio.h>

int main()
{
    FILE *fp;
    int Length;
    unsigned char buff[256] = {0};

    if((fp = fopen("./demo.c","r+t")) != NULL)
    {
        fseek(fp,0,SEEK_END);
        Length = ftell(fp);
        fclose(fp);   
    }

    printf("Length : %d\n",Length);

    return 0;
}

