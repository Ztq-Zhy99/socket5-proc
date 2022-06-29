#include "main.h"
#include "socks5_struct.h"

int main(int argc,char * argv[])
{
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_adr,clnt_adr;
    socklen_t clnt_adr_size;

    if(argc!=2)
    {
        printf("Usage:%s<IP>\n",argv[0]);
        exit(1);
    }

    //创建监听套接字
    if((serv_sock = socket(PF_INET,SOCK_STREAM,0)) == -1)
        error_handling("socket() error!\n");
    
    //初始化监听套接字的信息
    memset(&serv_adr,0,sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    //分配好监听套接字的信息
    if(bind(serv_sock,(struct sockaddr*)&serv_adr,sizeof(serv_adr)) == -1)
        error_handling("bind() error!\n");

    //使监听套接字处于可接收状态
    if(listen(serv_sock,BUF_SIZE) == -1)
        error_handling("listen() error!\n");
    
    clnt_adr_size = sizeof(clnt_adr);

    signal(SIGCHLD,sig_handle);

    while(1)
    {
        if((clnt_sock = accept(serv_sock,(struct sockaddr*)&clnt_adr,&clnt_adr_size) == -1))
            error_handling("accept() error!\n");

        if(fork() == 0)//子进程运行区域
        {
            //首先要关闭服务端的监听套接字
            close(serv_sock);

            //socks5建立连接
            int fd = handle_clnt_connect(clnt_sock);
            if(fd == -1)
                error_handling("handle_clnt_connect() error!\n");

            //连接建立成功之后传输信息
            if(communication(clnt_sock,fd) == -1)
                error_handling("communication() error!\n");
        }
        close(clnt_sock);
    }
    return 0;
}