#include "main.h"
#include "socks5_struct.h"

void error_handling(char * message)
{
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

void sig_handle(int sig)
{
    pid_t pid;
    int status;

    while(pid = (waitpid(-1,&status,WNOHANG)>0))
        printf("child process:%d terminaed!\n",pid);
    return;
}

//比较两个文件描述符的大小，后面好确定select监视范围是多大
int r_max(int clnt_fd,int serv_fd)
{
    return clnt_fd>serv_fd?clnt_fd:serv_fd;
}

//获取客户端握手信息
int get_clnt_hello_info(int fd,struct clinet_hello * clnt_hello)
{
    printf("get_clnt_hello_info start\n");
    char buf[BUF_SIZE];
    if(read(fd,buf,BUF_SIZE) == -1)
        error_handling("read() error!\n");

    clnt_hello->ver = buf[0];
    clnt_hello->nmethods = buf[1];
    memmove(clnt_hello->methods,&buf[2],1);
    printf("get_clnt_hello_info end\n");
} 

//读取客户端连接请求信息
int read_clnt_connect_info(int fd,struct client_connect_request * clnt_connect_request)
{
    printf("read_clnt_connect_info start\n");
    int length;//用于接收域名第一个字节
    struct hostent * host;
    unsigned char * tmp_adr = NULL;

    char buf[BUF_SIZE];
    if(read(fd,buf,BUF_SIZE) == -1)
        error_handling("read() error!\n");

    clnt_connect_request->VER = buf[0];
    clnt_connect_request->CMD = buf[1];
    clnt_connect_request->RSV = buf[2];
    clnt_connect_request->ATYP = buf[3];

    switch(clnt_connect_request->ATYP)
    {
        //IPv4
        case 0x01:
            memmove(clnt_connect_request->DST_ADDR,&buf[4],4);//IPv4占4字节
            memmove(clnt_connect_request->DST_PORT,&buf[8],2); //端口占2字节
            break;

        //域名处理
        case 0x03:
            length = buf[4];
            clnt_connect_request->adr_length = length;

            tmp_adr = (unsigned char *)malloc(length);
           
            memmove(tmp_adr,&buf[5],length);
            host = gethostbyname(tmp_adr);

            //只使用第一个IP地址
            memmove(clnt_connect_request->DST_ADDR,host->h_addr_list[0],4);

            //端口
            memmove(clnt_connect_request->DST_PORT,&buf[5+length],2);

            free(tmp_adr);
            break;

        //IPv6
        case 0x04:
            printf("IPv6\n");
            return -1;
            break;

        default:
            printf("地址类型有误!\n");
            break;
    }
    printf("read_clnt_connect_info end\n");
}


int serv_connect_dest_adr(struct client_connect_request * clnt_connect_info)
{
    printf("serv_connect_dest_adr start\n");
    struct sockaddr_in sock_adr;
    int remote_fd = socket(PF_INET,SOCK_STREAM,0);

    sock_adr.sin_family = AF_INET;
    memmove(&sock_adr.sin_addr.s_addr,clnt_connect_info->DST_ADDR,4);
    memmove(&sock_adr.sin_port,clnt_connect_info->DST_PORT,2);

    if(connect(remote_fd,(struct sockaddr *)&sock_adr,sizeof(sock_adr)) == -1)
        error_handling("connect() error!\n");
    
    return remote_fd;
    printf("serv_connect_dest_adr end\n");
}

//客户端与代理服务器建立socket5连接
int socket5_connect(int fd)
{
    printf("socket5_connect start\n");
    struct clinet_hello clnt_hello;
    struct server_hello serv_hello;
    struct client_connect_request clnt_connect_request_info;
    struct server_response serv_response_info;

    if(get_clnt_hello_info(fd,&clnt_hello) == -1) 
        error_handling("get_clnt_hello_info() error!\n");
    
    //判断获取的客户端握手信息中参数类型
    if(clnt_hello.ver != 0x05 && clnt_hello.methods[0] != 0x00)
    {
        printf("当前服务器只支持0x05 无验证方式！\n");
        return -1;
    }

    //返回服务端握手信息
    if(write(fd,(void *)&serv_hello,sizeof(serv_hello)) == -1)
        error_handling("write() error!\n");
    
    //读取客户端连接信息
    if(read_clnt_connect_info(fd, &clnt_connect_request_info) == -1)
        error_handling("read_clnt_connect_info() error!\n");
    
    //服务器与客户端建立连接
    int remote_fd = serv_connect_dest_adr(&clnt_connect_request_info);
    if(clnt_connect_request_info.CMD != 0x01)
    {
        printf("当前服务器仅支持connect!\n");
        return -1;
    }
    if(clnt_connect_request_info.ATYP == 0x04)
    {
        printf("当前服务器仅支持IPv4!\n");
        return -1;
    }

    //服务端回应客户端的连接信息
    serv_response_info.VER = clnt_connect_request_info.VER;
    serv_response_info.REP = 0x00; //连接成功
    serv_response_info.RSV = clnt_connect_request_info.RSV;
    serv_response_info.ATYP = 0x01; //采用IPv4
    memmove(serv_response_info.BND_ADDR,clnt_connect_request_info.DST_ADDR,4);
    memmove(serv_response_info.BND_PORT,clnt_connect_request_info.DST_PORT,2);

    if(write(fd,(void *)&serv_response_info,sizeof(serv_response_info)) == -1)
        error_handling("write() error!\n");

    return remote_fd;
    printf("socket5_connect end\n");
}

//客户端与代理服务器交换信息
int communication(int clnt_fd,int remote_fd)
{
    printf(" communication start\n");
    char data_buf[MAX_SIZE];
    int client_fd = clnt_fd;
    int serv_fd = remote_fd;
    fd_set fd1,fd2;
    FD_ZERO(&fd1);
    FD_ZERO(&fd2);
    FD_SET(client_fd,&fd1);//将客户端，服务端套接字都注册到fd_set数组中集中监视
    FD_SET(serv_fd,&fd1);

    int max_fd = r_max(client_fd,serv_fd)+1;//select的最大监视范围

    fd2 = fd1;
    
    while(1)
    {
        fd2 = fd1;
        if(select(max_fd,&fd2,NULL,NULL,NULL) < 0)
            error_handling("select() error!\n");

        //读取客户端数据，返回给服务端
        if(FD_ISSET(client_fd,&fd2))
        {
            int nbytes = read(client_fd,data_buf,MAX_SIZE); //读取client_fd套接字的内容到data_buf里面，最大字节数是MAX_SIZE

            if(nbytes < 0)//读取失败，返回错误
                error_handling("select read clinet_fd error!\n");

            if(nbytes == 0) //客户端的数据读取完毕，关闭客户端的读，关闭服务端的写
            {
                printf("read nbytes end:%d\n",nbytes);
                shutdown(client_fd,SHUT_RD);
                shutdown(serv_fd,SHUT_WR);
                FD_CLR(client_fd,&fd1);
                continue;
            }

            if(write(serv_fd,data_buf,nbytes) == -1)//将读取到的内容写给服务端
                error_handling("select write serv_fd error!\n");
        }
        

        //读取服务端数据，返回给客户端
        if(FD_ISSET(serv_fd,&fd2))
        {
            int nbytes = read(serv_fd,data_buf,MAX_SIZE);
            
            if(nbytes < 0)
                error_handling("select read serv_fd error!\n");
            
            if(nbytes == 0)
            {
                shutdown(serv_fd,SHUT_RD);
                shutdown(client_fd,SHUT_WR);
                break;
            }

            if(write(client_fd,data_buf,nbytes) < 0)
                error_handling("select write serv_fd error!\n");
            
        }
       // printf("11111111\n");
    }
    close(client_fd);
    close(remote_fd);
    printf("normal close!\n");
    printf(" communication end\n");
    return 1;
}

int handle_clnt_connect(int clnt_sock)
{
    printf("handle_clnt_connect start \n");
    int clnt_fd;
    clnt_fd = clnt_sock;

    int remote_fd = socket5_connect(clnt_fd);

    if(remote_fd == -1)
        error_handling("socket5() error!\n");

    return remote_fd;
    printf(" handle_clnt_connect end\n");
}

