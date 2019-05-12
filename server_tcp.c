#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>

int main(int argc, const char* argv[])
{
    // 创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lfd == -1)
    {
        perror("socket error");
        exit(1);
    }

    // lfd 和本地的IP port绑定
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;    // 地址族协议 - ipv4
    server.sin_port = htons(8888);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    int ret = bind(lfd, (struct sockaddr*)&server, sizeof(server));
    if(ret == -1)
    {
        perror("bind error");
        exit(1);
    }

    // 设置监听
    ret = listen(lfd, 20);
    if(ret == -1)
    {
        perror("listen error");
        exit(1);
    }

    // 等待并接收连接请求
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    int cfd = accept(lfd, (struct sockaddr*)&client, &len);
    if(cfd == -1)
    {
        perror("accept error");
        exit(1);
    }

    printf(" accept successful !!!\n");
    char ipbuf[64] = {0};
    printf("client IP: %s, port: %d\n", 
           inet_ntop(AF_INET, &client.sin_addr.s_addr, ipbuf, sizeof(ipbuf)),
           ntohs(client.sin_port));
    // 一直通信
    while(1)
    {
        // 先接收数据
        char buf[1024] = {0};
        int len = read(cfd, buf, sizeof(buf));
        if(len == -1)
        {
            perror("read error");
            exit(1);
        }
        else if(len == 0)
        {
            printf(" 客户端已经断开了连接 \n");
            close(cfd);
            break;
        }
        else
        {
            printf("recv buf: %s\n", buf);
            // 转换 - 小写 - 大写
            for(int i=0; i<len; ++i)
            {
                buf[i] = toupper(buf[i]);
            }
            printf("send buf: %s\n", buf);
            write(cfd, buf, len);
        }
    }

    close(lfd);

    return 0;
}
