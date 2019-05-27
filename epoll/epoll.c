#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/epoll.h>

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        printf("eg: ./a.out port\n");
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
    memset(&client_addr, 0, client_len);

    //创建epoll监听树
    int epfd = epoll_create(2000);

    //初始化epoll树
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

    struct epoll_event event[3000];
    while (1)
    {
        int ret = epoll_wait(epfd, event, sizeof(event)/sizeof(event[0]), -1);
        for (int i = 0; i < ret; ++i)
        {
            int fd = event[i].data.fd;
            if (fd == lfd) //监听套接字读到
            {
                int cfd = accept(lfd, (struct sockaddr *)&client_addr, &client_len);
                if (cfd == -1)
                {
                    perror("accept error\n");
                    exit(1);
                }
                char ip[64] = {0};
                ev.events = EPOLLIN;
                ev.data.fd = cfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
                printf("client ip : %s, port : %d \n",
                inet_ntop(AF_INET,&client_addr.sin_addr.s_addr,ip,sizeof(ip)),
                ntohs(serv_addr.sin_port));
            }
            else
            {
                if (!fd & EPOLLIN)
                {
                    continue;
                }
                else
                {
                    char buf[1024] = {0};
                    int len = recv(fd,buf,sizeof(buf),0);
                    if(len == -1)
                    {   
                        perror("recv error\n");
                        exit(1);
                    }
                    else if (len==0)
                    {
                        printf("client disconnected....\n");
                        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                        close(fd);
                    }
                    else
                    {
                        printf("recv buf: %s\n",buf);
                        write(fd,buf,sizeof(buf));
                    }
                    
                    
                }
                
            }
        }
    }
    close(lfd);
    return 0;
}
