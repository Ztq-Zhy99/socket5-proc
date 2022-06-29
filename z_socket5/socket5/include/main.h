#ifndef __MAIN_H__
#define __MAIN_H__

#include<stdio.h>
#include<stdlib.h>
#include<sys/select.h>
#include<sys/time.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/wait.h>

#define BUF_SIZE 1024
#define MAX_SIZE 1024

//客户端连接请求处理
int handle_clnt_connect(int clnt_sock);

//交换数据处理
int communication(int clnt_sock, int fd);

//子进程信号处理
void sig_handle(int sig);

#endif 