//
// Created by wangkaikai on 19-5-31.
//

#ifndef NETWORK_CODE_EPOLL_WEBSERVER_H
#define NETWORK_CODE_EPOLL_WEBSERVER_H

void epoll_run(int port);
int init_listen_fd(int port,int epfd);
void do_accept(int lfd,int epfd);
int get_line(int sock, char *buf, int size);
void disconnect(int cfd, int epfd);
void http_request(const char *request,int cfd);
void do_read(int cfd, int epfd);
void send_dir(int cfd, const char *dirname);
void send_respond_head(int cfd, int number, const char *desp, const char *type, long len);
void send_file(int cfd, char *filename);
void encode_str(char* to, int tosize, const char* from);
int hexit(char c);
void decode_str(char *to,char *from);
const char *get_file_type(const char *name);

#endif //NETWORK_CODE_EPOLL_WEBSERVER_H
