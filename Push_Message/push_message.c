#include "push_message.h"

char gUID[24] = "1234567890";
char push_server_path[64] = {0};
unsigned char feed_list[84] = {0};

int vender_type = 0;
struct sockaddr_in gPushMsgSrvAddr;

char *GetPushMessageString(char *UID,unsigned char *list)
{
    static char msgBuf[2048];

    sprintf(msgBuf, "GET /%s/server.php?cmd=raisealarm&devid=%s&feedlist=%s\r\nHTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: keep-alive\r\n"
            "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/536.5 (KHTML, like Gecko) Chrome/19.0.1084.52 Safari/536.5\r\n"
            "\r\n", push_server_path, UID,list,inet_ntoa(gPushMsgSrvAddr.sin_addr));

    return msgBuf;
}

int SendPushMessage(unsigned char *list)
{
    int ret = 0;

    int skt;

    if(gPushMsgSrvAddr.sin_addr.s_addr == 0)
    {
        printf("No push message server\n");
        SendRegister();
//        return ret;
    }

    if((skt =(int)socket(AF_INET, SOCK_STREAM, 0)) >= 0)
    {
        printf("server_addr:%s\n",inet_ntoa(gPushMsgSrvAddr.sin_addr));
//        printf("server_sin_port:%d\n",gPushMsgSrvAddr.sin_port);
        int time_out = 6000; // 2秒

        if(connect(skt, (struct sockaddr *)&gPushMsgSrvAddr, sizeof(struct sockaddr_in)) == 0)
        {
            printf("connect HTTP OK\n");
//            printf("server_addr:%s\n",inet_ntoa(gPushMsgSrvAddr.sin_addr));

            char *msg = GetPushMessageString(gUID,list);

            setsockopt(skt, SOL_SOCKET,SO_SNDTIMEO, (char *)&time_out,sizeof(time_out));

            send(skt, msg, strlen(msg), 0);

            printf("send mesg:%s\n", msg);

            if(vender_type >= 6)
            {
                char buf[1024]={0};
                time_out = 6000; // 2秒

                setsockopt(skt, SOL_SOCKET,SO_RCVTIMEO, (char *)&time_out,sizeof(time_out));
                recv(skt, buf, 1024, 0);
                printf("revice buf:%s\n", buf);

                ret = 1;
            }
            else
            {
                ret = 1;
            }
        }
        else
        {
            ret = 0;
            printf("connect tcp error server_addr:%s\n",inet_ntoa(gPushMsgSrvAddr.sin_addr));
        }

        sleep(10);
        close(skt);
    }

    return ret;
}

char *GetRegMessageString(char *UID)
{
    static char msgBuf[2048]={0};

    if(vender_type == 1)
    {
        sprintf(msgBuf, "GET /tpns?cmd=device&uid=%s\r\nHTTP/1.1\r\n"
                "Host: %s\r\n"
                "Connection: keep-alive\r\n"
                "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/536.5 (KHTML, like Gecko) Chrome/19.0.1084.52 Safari/536.5\r\n"
                "\r\n", UID, inet_ntoa(gPushMsgSrvAddr.sin_addr));
    }
    else
    {
        sprintf(msgBuf, "GET /apns/apns.php?cmd=reg_server&uid=%s\r\nHTTP/1.1\r\n"
                "Host: %s\r\n"
                "Connection: keep-alive\r\n"
                "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/536.5 (KHTML, like Gecko) Chrome/19.0.1084.52 Safari/536.5\r\n"
                "\r\n", UID, inet_ntoa(gPushMsgSrvAddr.sin_addr));
    }

    printf("msgBuf:%s\n", msgBuf);
    return msgBuf;
}

int SendRegister()
{
    int port_remote =  0;
    struct hostent *host;

    memset(push_server_path, 0, sizeof(push_server_path));

    struct_pushserver info;
    char server_addr[128]={0};

    info.ip_port = 8088;

    memcpy(server_addr,"120.24.215.192", sizeof(server_addr));
    memset(push_server_path, 0, sizeof(push_server_path));
    memcpy(push_server_path, "Test", sizeof(push_server_path));
    port_remote = info.ip_port;

    host = gethostbyname((char*)server_addr);

    printf("vender_type:%d, push_server_path:%s, server_addr:%s\n", vender_type, push_server_path, server_addr);
    printf("ip_port:%d\n",port_remote);

    if(host != NULL)
    {
        memcpy(&gPushMsgSrvAddr.sin_addr, host->h_addr_list[0], host->h_length);

        gPushMsgSrvAddr.sin_port = htons(8088);

        if(port_remote != 0)
        {
            gPushMsgSrvAddr.sin_port = htons(port_remote);
        }
        gPushMsgSrvAddr.sin_family = AF_INET;
    }
    else
    {
        printf("faile to resolve\n");
        memset(&gPushMsgSrvAddr, 0, sizeof(gPushMsgSrvAddr));
        return 0;
    }

//    printf("port_remote ip_port:%d\n",port_remote);
//    printf("sin_port ip_port:%d\n",gPushMsgSrvAddr.sin_port);

    int skt;
    if((skt =(int) socket(AF_INET, SOCK_STREAM, 0)) >= 0)
    {
        if(connect(skt, (struct sockaddr *)&gPushMsgSrvAddr, sizeof(struct sockaddr_in)) == 0)
        {
            char *msg = GetRegMessageString(gUID);
            printf("%s", msg);
            send(skt, msg, strlen(msg), 0);
            char buf[1024]={0};
            recv(skt, buf, 1024, 0);
            //printf("Reg = %s\n", buf);
            printf("Register OK\n");
        }

        close(skt);
        return 1;
    }
}

int main()
{
    int i;

    for(i = 0;i < 80;i++)
    {
        feed_list[i] = 1 + '0';
    }

    printf("feed_list : %s\n",feed_list);
    SendPushMessage(feed_list);
    return 0;
}
