#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>

int main(int argc, char const *argv[])
{
    //创建套接字
    int fd = socket(AF_INET,SOCK_DGRAM,0);
    if(fd == -1)
    {
        perror("socket error\n");
        exit(1);
    }
    //绑定本地的ip和端口
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(8765);

    int ret = bind(fd,(struct sockaddr*)&server_addr,sizeof(server_addr));
    if(ret == -1)
    {
        perror("bind error\n");
        exit(1);
    } 
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    memset(&client_addr,0,sizeof(client_addr));
    char buf[1024] = {0};
    while(1)
    {
        int recvlen = recvfrom(fd,buf,sizeof(buf),0,(struct sockaddr*)&client_addr,&client_len);
        if(recvlen == -1)
        {
            perror("recvfrom error\n");
            exit(1);
        }
        printf("recv buf:%s\n",buf);
        char ip[64] = {0};
        printf("client ip address : %s, port : %d\n",
        inet_ntop(AF_INET,&client_addr.sin_addr.s_addr,ip,sizeof(ip)),
        ntohs(client_addr.sin_port));
        sendto(fd,buf,recvlen,0,(struct sockaddr*)&client_addr,client_len);
    }
    close(fd);
    return 0;
}
