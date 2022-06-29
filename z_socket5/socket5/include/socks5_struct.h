#ifndef __SOCKS5_STRUCT_H__
#define __SOCKS5_STRUCT_H__

struct clinet_hello 
{
    unsigned char ver;
    unsigned char nmethods;
    unsigned char methods[255];
};

struct server_hello 
{
    unsigned char ver;
    unsigned char method;
};

struct client_connect_request
{
    unsigned char VER;
    unsigned char CMD;
    unsigned char RSV;
    unsigned char ATYP;

    unsigned int adr_length;  //存储域名字节大小

    unsigned char DST_ADDR[4];
    unsigned char DST_PORT[2]; 
};

struct server_response
{
    unsigned char VER;
    unsigned char REP;
    unsigned char RSV;
    unsigned char ATYP;
    unsigned char BND_ADDR[4];
    unsigned char BND_PORT[2];
};

#endif