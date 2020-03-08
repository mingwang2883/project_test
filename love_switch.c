#include <stdio.h>
#include <stdlib.h>
#include <time.h>




int main(int argc,char *argv[])
{
    int ret = -1;
    int rand_num;
    unsigned int inp_num;
    time_t r_time;

    time(&r_time);
    srand(r_time);
    rand_num = rand();
    printf("rand_num : %d \n",rand_num);
    printf("请输入你的选择:");
    ret = scanf("%d",&inp_num);
    if (ret == 0)
    {
        printf("scanf error !\n");
        return -1;
    }
    printf("%d\n",inp_num);
choose_1:
    switch(inp_num)
    {
    case 0x01:
        break;
    case 0x02:
        break;
    case 0x03:
        break;
    case 0x04:
        break;
    case 0x05:
        break;
    case 0x06:
        break;
    default:
        printf("no this choose\n");
        break;
    }
	
	return 0;
}
