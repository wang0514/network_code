//
// Created by wangkaikai on 19-5-30.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/epoll.h>
#include "epoll_webserver.h"

#define MAXSIZE 2000


void epoll_run(int port)
{
    //创建一个epoll树的根节点
    int epfd = epoll_create(MAXSIZE);
    if (epfd == -1)
    {
        perror("epoll create error\n");
        exit(1);
    }
    //添加要监听的节点
    //先添加监听的lfd
    int lfd = init_listen_fd(port, epfd);
    //委托内核进行检测
    struct epoll_event all[MAXSIZE];
    all[0].events = EPOLLIN;
    while (1)
    {
        int ret = epoll_wait(epfd, all, MAXSIZE, -1);
        if (ret == -1)
        {
            perror("epoll wait error\n");
            exit(1);
        }

        //遍历发生变化的节点
        for (int i = 0; i < ret; i++)
        {
            //只处理读事件，其他事件默认不处理
            struct epoll_event *pev = &all[i];
            if (!(pev->events & EPOLLIN))
            {
                //不是读事件
                continue;
            }
            if (pev->data.fd == lfd)
            {
                //接受连接请求
                do_accept(lfd, epfd);
            }
            else
            {
                //读取数据
                do_read(pev->data.fd, epfd);
            }
        }
    }
}

//读数据
void do_read(int cfd, int epfd)
{
    //将浏览器发过来的数据，读到buf中
    char line[1024] = {0};
    int len = get_line(cfd, line, sizeof(line));
    if (len == 0)
    {
        printf("client disconnected....\n");
        //关闭套接字，cfd从epoll上删除
        disconnect(cfd, epfd);
    }
    else
    {
        printf("请求行数据 :%s", line);
        printf("===================请求头========================\n");
        //还没有读完
        //继续读f
        while (len>0)
        {
            char buf[1024] = {0};
            len = get_line(cfd, buf, sizeof(buf));
            printf("--------：%s", buf);
        }
        printf("===============the end =============\n");
    }
    //请求行： get /xxx http/1.1
    //判断是不是get请求
    if (strncasecmp("get", line, 3) == 0)
    {
        //处理http请求
        http_request(line,cfd);
        disconnect(cfd,epfd);
    }
}

//http请求处理
void http_request(const char *request, int cfd)
{
    //拆分http请求行

    char method[12], path[1024], protocol[12];
    sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
    printf("method = %s, path = %s, protocol = %s\n", method, path, protocol);

    //转码
    decode_str(path,path);
    //处理path /xxx
    char *file = path + 1;
    //如果没有指定访问的资源，默认使用显示资源根目录的内容
    if(strcmp(path,"/")==0)
    {
        //file值资源目录的当前位置
        file = "./";
    }

    //获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1)
    {
        //show 404
        send_respond_head(cfd,404,"Not found",".html",-1);
        send_file(cfd,"404 Not Found.html");
    }

    //判断是目录还是文件
    //如果是目录
    if (S_ISDIR(st.st_mode))
    {
        //发送头信息
        send_respond_head(cfd, 200, "OK", get_file_type(".html"),-1);
        //发送目录信息
        send_dir(cfd,file);
    }
    else if (S_ISREG(st.st_mode))
    {
        //文件
        //发送消息抱头的函数
        send_respond_head(cfd, 400, "OK", get_file_type(file), st.st_size);
        //发送文件的内容
        send_file(cfd, file);
    }
}

//发送目录内容
void send_dir(int cfd, const char *dirname)
{
    //拼一个ｈｔｍｌ页面<ｔａｂｌｅ></table>
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>目录名: %s</title></head>", dirname);
    sprintf(buf + strlen(buf), "<body><h1>当前目录：%s</h1><table>", dirname);
    char enstr[1024];
    char path[1024] = {0};
    //目录项二级指针
    struct dirent** ptr;
    int num = scandir(dirname,&ptr,NULL,alphasort);
    //遍历
    for(int i = 0;i<num;++i)
    {
        char* name = ptr[i]->d_name;
        //拼接文件的完整路径
        sprintf(path,"%s/%s",dirname,name);
        printf("path = %s==============\n",path);
        struct stat st;
        stat(path,&st);

        encode_str(enstr, sizeof(enstr),name);
        //如果是文件
        if(S_ISREG(st.st_mode))
        {

            sprintf(buf+strlen(buf),"<tr><td><a href = \"%s\">%s</a><td>%ld</td></td></tr>",enstr,name,st.st_size);
        }
            //如果是目录
        else if(S_ISDIR(st.st_mode))
        {
            sprintf(buf+strlen(buf),"<tr><td><a href = \"%s/\">%s/</a><td>%ld</td></td></tr>",enstr,name,st.st_size);
        }
        send(cfd,buf,strlen(buf),0);
        memset(buf,0,sizeof(buf));
    }
    sprintf(buf+strlen(buf),"</table></body></html>");
    send(cfd,buf,strlen(buf),0);
    printf("send message ok!!!!\n");
#if 0
    //打开目录
    DIR *dir = open(dirname);
    if (dir == NULL)
    {
        perror("open dir error\n");
        exit(1);
    }

    //读目录
    struct dirent* ptr = NULL;
    while((ptr = readdir(dir))!=NULL)
    {
        char* name = ptr->d_name;
    }
    closedir(dir);
#endif
}
//发送响应头
void send_respond_head(int cfd, int number, const char *desp, const char *type, long len)
{
    char buf[1024] = {0};
    //状态行
    sprintf(buf, "HTTP/1.1 %d %s\r\n", number, desp);
    send(cfd, buf, sizeof(buf), 0);
    //消息报头
    sprintf(buf, "Content-Type:%s\r\n", type);
    sprintf(buf + strlen(buf), "Content-Length: %ld\r\n", len);
    send(cfd, buf, strlen(buf), 0);
    //空行
    send(cfd, "\r\n", 2, 0);
}

//发送文件
void send_file(int cfd, char *filename)
{
    //打开文件
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        //show404
        send_respond_head(cfd,404,"Not found",".html",-1);
        send_file(cfd,"404 Not Found.html");
        return;
    }
    //循环读文件
    char buf[4094] = {0};
    int len = 0;
    while ((len = read(fd, buf, sizeof(buf))) > 0)
    {
        //发送读出来的数据
        send(cfd, buf, len, 0);
    }
    if (len == -1)
    {
        perror("read file error\n");
        exit(1);
    }
    close(fd);
}

//断开链接的函数
void disconnect(int cfd, int epfd)
{
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
    if (ret == -1)
    {
        perror("epoll_del error\n");
        exit(1);
    }
    close(cfd);
}
//解析http请求信息的每一行内容
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                {
                    recv(sock, &c, 1, 0);
                }
                else
                {
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        }
        else
        {
            c = '\n';
        }
    }
    buf[i] = '\0';
    if (n == -1)
    {
        i = -1;
    }
    return (i);
}

//接受新连接请求
void do_accept(int lfd, int epfd)
{
    struct sockaddr_in client;
    socklen_t clientlen = sizeof(client);
    int cfd = accept(lfd, (struct sockaddr *)&client, &clientlen);
    if (cfd == -1)
    {
        perror("accept error\n");
        exit(1);
    }
    char ip[64] = {0};
    printf("client ip address: %s, port : %d,cfd:%d\n",
           inet_ntop(AF_INET, &client.sin_addr.s_addr, ip, sizeof(ip)),
           ntohs(client.sin_port), cfd);

    //设置cfd为非阻塞
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    //得到的新节点挂到epoll树上
    struct epoll_event ev;
    //边缘非阻塞模式
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = cfd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
    if (ret == -1)
    {
        perror("epoll add error\n");
        exit(1);
    }
}
int init_listen_fd(int port, int epfd)
{
    //初始化监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket error\n");
        exit(1);
    }
    //lfd绑定本地的ip和端口
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    //端口复用
    int flag = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    int ret = bind(lfd, (struct sockaddr *)&server, sizeof(server));
    if (ret == -1)
    {
        perror("bind error\n");
        exit(1);
    }
    //设置监听
    ret = listen(lfd, 64);
    if (ret == -1)
    {
        perror("listen error\n");
        exit(1);
    }

    //lfd添加到epoll树上

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if (ret == -1)
    {
        perror("epoll_ctl error\n");
        exit(1);
    }
    return lfd;
}

//解码
/*
 *  这里的内容是处理%20之类的东西！是"解码"过程。
 *  %20 URL编码中的‘ ’(space)
 *  %21 '!' %22 '"' %23 '#' %24 '$'
 *  %25 '%' %26 '&' %27 ''' %28 '('......
 *  相关知识html中的‘ ’(space)是&nbsp
 */
void encode_str(char* to, int tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from)
    {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0)
        {
            *to = *from;
            ++to;
            ++tolen;
        }
        else
        {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}
// 16进制数转化为10进制
int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}


//编码,用作回写浏览器的时候,将除数字以及/_.-~以外的字符转义后回写
void decode_str(char *to,char *from)
{
    for(;*from!='\0';++to,++from)
    {
        if(from[0] == '%'&&isxdigit(from[1])&&isxdigit(from[2]))
        {
            //依次判断from中%20三个字符
            *to = hexit(from[1])*16+hexit(from[2]);
            //移过已经处理的两个字符（%21指针指向1），表达式3的++from还会再向后移动一个字符
            from+=2;
        }
        else
        {
            *to = *from;
        }
    }
    *to = '\0';
}

const char *get_file_type(const char *name)
{
    char* dot;

    // 自右向左查找‘.’字符, 如不存在返回NULL
    dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav" ) == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}
