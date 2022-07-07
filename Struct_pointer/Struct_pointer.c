#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct
{
    int id;
    int index;
    char name[20];
}SString;

typedef struct
{
    int num;
    int total;
    SString str[0];
}STest;

void struct_malloc(STest **pstr)
{
    int size;
    STest *str = NULL;

    size = sizeof(SString) + sizeof(STest);
    str = (STest *)malloc(size);

    memset(str,0,size);

    str->num   = 15;
    str->total = 20;
    str->str[0].id = 5;

    *pstr = str;
}

int main(void)
{
    STest *t;

    struct_malloc(&t);

    printf("t.num : %d\n",t->num);
    printf("t.total : %d\n",t->total);
    printf("t->str[0].id : %d\n",t->str[0].id);

    if(t != NULL)
    {
        free(t);
    }

    return 0;
}

