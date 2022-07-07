#include <stdio.h>

typedef int (* my_cb_func)(int value);

typedef struct
{
    int val;
    my_cb_func cb;
}StructCb,*pStructCb;

int show_val(int val)
{
    printf("val : %d\n",val);
    return 0;
}

int function(pStructCb cb)
{
    cb->cb(cb->val);
}

int main()
{
    StructCb s_cb;

    s_cb.val = 1;
    s_cb.cb = show_val;

    function(&s_cb);

    return 0;
}
