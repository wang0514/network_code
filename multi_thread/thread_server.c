#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>

typedef struct SockInfo
{
    int fd;
    struct sockaddr_in addr;
    pthread_t id;
}SockInfo;

void *handler(void* arg)
{
    SockInfo* info  = (SockInfo *)arg;
    char ip[64];
    char buf[1024];
    //通信
    while (1)
    {
        printf("ip address : %s, port :%d\n",
        inet_ntop(AF_INET,&info->addr.sin_addr.s_addr,ip,sizeof(ip)),
        ntohs(info->addr.sin_port));
        int len = read(info->fd,buf,sizeof(buf));
        if(len == -1)
        {
            perror("read error\n");
            pthread_exit(NULL);
        }
        else if (len == 0)
        {       
            printf("客户端断开了连接\n");
            close(info->fd);
            break;
        }
        else
        {
            printf("%s\n",buf);
            write(info->fd,buf,sizeof(buf));
        }
    }
    
}
int main(int argc, char const *argv[])
{
    if(argc<2)
    {
        printf("eg:./a.out port");
        exit(1);
    }
    int lfd = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in servaddr;
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htonl(atoi(argv[1]));
    socklen_t serlen= sizeof(servaddr);

    bind(lfd,(struct sockaddr*)&servaddr,serlen);

    listen(lfd,36);
    printf("start accept...\n");

    int i = 0;
    SockInfo info[256];
    for(i = 0;i<sizeof(info)/sizeof(info[0]);++i)
    {
        info[i].fd = -1;
    }

    socklen_t client_len = sizeof(struct sockaddr_in);
    while (1)
    {
        for(i = 0;i<sizeof(info)/sizeof(info[0]);++i)
        {
            if(info[i].fd ==-1)
            {
                break;
            }
        }
        if(i == 256)
        {
            break;
        }
        info[i].fd = accept(lfd,(struct sockaddr*)&info[i].addr,&client_len);

        //创建子线程
        pthread_create(&info[i].id,NULL,handler,&info[i]);
        pthread_detach(info[i].id);
    }
    close(lfd);
    
    //只退出主线程
    pthread_exit(NULL);
    
    return 0;
}
