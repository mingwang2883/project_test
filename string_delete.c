#include <stdio.h>
#include <string.h>

int main()
{
    char src_mac[24] = "12:34:56:78:90:12";
    char mac[24] = {0};
    char mac_str[16] = {0};
    char *p_mac = mac;
    char *p_mac_str = mac_str;

    strcpy(mac,src_mac);
    while(*p_mac != '\0')
    {
        if(*p_mac == ':')
        {
            p_mac++;
            continue;
        }
        *p_mac_str = *p_mac;
        p_mac++;
        p_mac_str++;
    }

    printf("mac = %s\n",mac_str);
    return 0;
}

