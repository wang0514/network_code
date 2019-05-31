//
// Created by wangkaikai on 19-5-31.
//
//
// Created by wangkaikai on 19-5-30.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "epoll_webserver.h"

int main(int argc, char const *argv[])
{
    if(argc<3)
    {
        printf("ed: ./a.out port path\n");
        exit(1);
    }
    //端口
    int port  = atoi(argv[1]);
    //修改进程的工作目录，方便后续操作
    int ret  = chdir(argv[2]);
    if(ret == -1)
    {
        perror("chdir error\n");
        exit(1);
    }
    //启动epoll模型
    epoll_run(port);
    return 0;
}


