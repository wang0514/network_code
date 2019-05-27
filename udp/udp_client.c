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
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        perror("socket error\n");
        exit(1);
    }
    //初始化服务器的ip和端口
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8765);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);
    while (1)
    {
        char buf[1024] = {0};
        fgets(buf, sizeof(buf), stdin);
        //数据的发送--serv ip port
        sendto(fd, buf, sizeof(buf), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        //等待服务器发送数据过来
        recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);
        printf("recv buf:%s\n", buf);
    }
    close(fd);
    return 0;
}
