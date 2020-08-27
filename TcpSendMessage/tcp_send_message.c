#include "tcp_send_message.h"

#define USE_WIFI_DEV    "wlan0"

#define SLEEP_SECOND_WAIT   2

//#define PHONE_IP_ADDRESS    "192.168.43.1"  /* mobile phone's ip */
#define PHONE_IP_ADDRESS    "192.168.5.63"  /* mobile phone's ip */
#define PHONE_SERVER_PORT   20000

#define	INFO_RET_FORMAT     "{\"uuid\":\"%s\",\"key\":\"%s\",\"mac\":\"%s\"}"

typedef struct
{
    unsigned char uuid[24];
    unsigned char key[36];
    unsigned char mac[20];
}StructDeviceInfo;

int get_uuid_and_authkey(unsigned char *uuid,unsigned char *key)
{
    FILE *UUID_fp;
    FILE *AUTHKEY_fp;
    int UUID_len,AUTHKEY_len;

    UUID_fp = popen("cat /mnt/config/tuya_config.txt | grep UUID | awk -F \"UUID=\" '{print $2}'","r");
    if(!UUID_fp)
    {
        perror("popen UUID_fp");
        return -1;
    }
    fgets(uuid,24,UUID_fp);
    UUID_len = strlen(uuid);
    uuid[UUID_len-1] = '\0';
    printf("GET SERVER UUID = %s\n",uuid);

    pclose(UUID_fp);

    AUTHKEY_fp = popen("cat /mnt/config/tuya_config.txt | grep AUTHKEY | awk -F \"AUTHKEY=\" '{print $2}'","r");
    if(!AUTHKEY_fp)
    {
        perror("popen AUTHKEY_fp");
        return -1;
    }
    fgets(key,36,AUTHKEY_fp);
    AUTHKEY_len = strlen(key);
    key[AUTHKEY_len-1] = '\0';
    printf("GET SERVER AUTHKEY = %s\n",key);

    pclose(AUTHKEY_fp);

    return 0;
}

int get_wifi_mac(char *mac)
{
    struct ifreq ifreq;
    int sock;

    if((sock=socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror("socket");
        return -2;
    }

    strcpy(ifreq.ifr_name,USE_WIFI_DEV);
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq) < 0)
    {
        perror("ioctl");
        return -3;
    }

    sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",
           (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
           (unsigned char)ifreq.ifr_hwaddr.sa_data[1],
           (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
           (unsigned char)ifreq.ifr_hwaddr.sa_data[3],
           (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
           (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);

    return 0;
}

void set_reuse_addr(int sockfd,int opt)
{
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt, sizeof(opt));
}

int connectToSever(int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket创建出错！");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(PHONE_IP_ADDRESS);

    printf("phone ip :%s port:%d\n",PHONE_IP_ADDRESS,port);
    bzero(&(serv_addr.sin_zero),8);
    if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr)) == -1)
    {
        perror("connect error！");
        return -1;
    }
    else
    {
        set_reuse_addr(sockfd, 1);
        return sockfd;
    }
}

int main(void)
{
    int size = 0;
    int len = 0;
    int sockfd = 0;
    unsigned char req[256] = {0};
    unsigned char resp[256] = {0};
    StructDeviceInfo dev_info;

    memset(&dev_info,0,sizeof(dev_info));
    sockfd = connectToSever(PHONE_SERVER_PORT);
restart:
    if(sockfd <= 0)
    {
        int count_times = 0;

        do
        {
            if(count_times++ >= 60) /* 2 minites */
            {
                printf("connect too long!!\n");
                return -1;
            }

            sleep(SLEEP_SECOND_WAIT);

            sockfd = connectToSever(PHONE_SERVER_PORT);
            printf("retry times : %d\n", count_times);

        }while(sockfd <= 0);
    }

    sleep(1);

    get_wifi_mac(dev_info.mac);
    get_uuid_and_authkey(dev_info.uuid,dev_info.key);
    sprintf(req, INFO_RET_FORMAT,dev_info.uuid,dev_info.key,dev_info.mac);

    len = strlen(req);
    size = write(sockfd, req, len);
    printf("write size : %d\n", size);

    if(size >= len)
    {
        printf("waiting app answer !\n");
        int ret = read(sockfd, resp, sizeof(resp));
        if((ret > 0) && (strstr(resp, "OK") != NULL))
        {
            printf("%s recive : %s\n",__FUNCTION__,resp);
            close(sockfd);
            sockfd = -1;
            return 0;
        }
        else
        {
            printf("send again %d\n",__LINE__);
            close(sockfd);
            sockfd = -1;
            goto restart;
        }
    }
    else
    {
        printf("send again %d\n",__LINE__);
        close(sockfd);
        sockfd = -1;
        goto restart;
    }

    close(sockfd);
    return 0;
}
