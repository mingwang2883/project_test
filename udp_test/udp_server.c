#include <stdio.h> /* These are the usual header files */
#include <string.h>
#include <unistd.h> /* for close() */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT                    8888 /* Port that will be opened */
#define MAXDATASIZE             1024 /* Max number of bytes of data */
#define CMD_STRING              "{\"cmd\":0}"
#define REPORT_CMD_STRING       "{\"cmd\":0,\"pid\":\"%s\",\"uid\":\"%s\",\"key\":\"%s\"}"

char dev_pid[24];
char dev_uuid[24];
char dev_key[36];

int get_pid_uuid_key(void)
{
    FILE *UUID_fp,*AUTHKEY_fp,*PID_fp;
    int UUID_len,AUTHKEY_len,PID_len;

    memset(dev_pid,0,sizeof(dev_pid));
    memset(dev_uuid,0,sizeof(dev_uuid));
    memset(dev_key,0,sizeof(dev_key));

    PID_fp = popen("cat /mnt/config/jiake_version.txt | grep PID | awk -F \"PID=\" '{print $2}'","r");
    if(!PID_fp)
    {
        perror("popen PID_fp");
        return -1;
    }
    fgets(dev_pid,sizeof(dev_pid),PID_fp);
    PID_len = strlen(dev_pid);
    dev_pid[PID_len-1] = '\0';
    printf("dev_pid = %s\n",dev_pid);

    pclose(PID_fp);

    UUID_fp = popen("cat /mnt/config/tuya_config.txt | grep UUID | awk -F \"UUID=\" '{print $2}'","r");
    if(!UUID_fp)
    {
        perror("popen UUID_fp");
        return -1;
    }
    fgets(dev_uuid,sizeof(dev_uuid),UUID_fp);
    UUID_len = strlen(dev_uuid);
    dev_uuid[UUID_len-1] = '\0';
    printf("dev_uuid = %s\n",dev_uuid);

    pclose(UUID_fp);

    AUTHKEY_fp = popen("cat /mnt/config/tuya_config.txt | grep AUTHKEY | awk -F \"AUTHKEY=\" '{print $2}'","r");
    if(!AUTHKEY_fp)
    {
        perror("popen AUTHKEY_fp");
        return -1;
    }
    fgets(dev_key,sizeof(dev_key),AUTHKEY_fp);
    AUTHKEY_len = strlen(dev_key);
    dev_key[AUTHKEY_len-1] = '\0';
    printf("dev_key = %s\n",dev_key);

    pclose(AUTHKEY_fp);

    return 0;
}

int main()
{
    int ret;
    int num;
    int sockfd; /* socket descriptors */
    struct sockaddr_in server; /* server's address information */
    struct sockaddr_in client; /* client's address information */
    socklen_t sin_size;
    char recvmsg[MAXDATASIZE]; /* buffer for message */
    char sendmsg[MAXDATASIZE];

    ret = get_pid_uuid_key();
    if(ret != 0)
    {
        printf("get_pid_uuid_key failed\n");
    }

    sin_size = sizeof(struct sockaddr_in);

    /* Creating UDP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        /* handle exception */
        perror("Creating socket failed.");
        exit(1);
    }

    bzero(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htons(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        /* handle exception */
        perror("Bind error.");
        exit(1);
    }

    while (1)
    {
        num = recvfrom(sockfd,recvmsg,MAXDATASIZE,0,(struct sockaddr *)&client,&sin_size);
        if (num < 0)
        {
            perror("recvfrom error\n");
            exit(1);
        }
        recvmsg[num] = '\0';

        if(strcmp(recvmsg,CMD_STRING) == 0)
        {
            printf("report device info\n");
            sprintf(sendmsg,REPORT_CMD_STRING,dev_pid,dev_uuid,dev_key);
            sendto(sockfd,sendmsg,strlen(sendmsg),0,(struct sockaddr *)&client,sin_size);
            break;
        }
    }

    close(sockfd); /* close listenfd */
}
