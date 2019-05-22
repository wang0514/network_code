#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/select.h>

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        perror("eg: ./out port");
        exit(1);
    }
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(lfd, 36);
    printf("start accept...\n");
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));

    int ret;
    //最大的文件描述符
    int maxfd = lfd;
    fd_set reads, temps;
    //初始化内核监测表
    FD_ZERO(&reads);
    FD_SET(lfd, &reads);
    char buf[1024] = {0};
    while (1)
    {
        temps = reads;
        ret = select(maxfd+1,&temps,NULL,NULL,NULL);
        if(ret == -1)
        {
            perror("select  error\n");
            exit(1);
        }
        //判断是否有新连接
        if(FD_ISSET(lfd,&temps))
        {
            //接受连接请求
            int cfd = accept(lfd,(struct sockaddr*)&client_addr,&client_len);
            if(cfd == -1)
            {
                perror("accept error\n");
                exit(1);
            }
            char ip[64];
            printf("New ip : %s, port : %d\n",
            inet_ntop(AF_INET,&client_addr.sin_addr.s_addr,ip,sizeof(ip)),
            ntohs(client_addr.sin_port));
            FD_SET(cfd,&reads);
            maxfd = maxfd<cfd?cfd:maxfd;
        }
        //遍历检测文件描述符是否有读操作

        for (int i = lfd+1; i <= maxfd; ++i)
        {
            if(FD_ISSET(i,&temps))
            {
                //读数据
                int len = read(i,buf,sizeof(buf));
                if(len == -1)
                {
                    perror("read error\n");
                    exit(1);
                }
                else if (len == 0)
                {
                    printf("客户端断开了连接\n");
                    close(i);
                    FD_CLR(i,&reads);
                }
                else
                {
                    printf("recv buf :%s\n",buf);
                    write(i,buf,sizeof(buf));
                }
                
            }
        }
        
    }

    close(lfd);
    return 0;
}
