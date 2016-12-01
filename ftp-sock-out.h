#define _CRT_SECURE_NO_WARNINGS

#ifndef _FTP_SOCK_OUT_
#define _FTP_SOCK_OUT_

#include<stdio.h>
#include<process.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<time.h>

#pragma comment(lib,"ws2_32.lib")

typedef SOCKET sock;

#define _PORT_ 0
#define _PASV_ 1
#define _K_ 1024
#define _M_ (1024*_K_)

extern sock ctrl_sk;
extern int transfer_mode;

int request_ftp_server(sock sk, const char *cmd);

int connect_ftp_server(char *ip_addr, u_short port);

void ctrl_ftp(char *ftp_cmd);

void recv_file(const char *ftp_cmd, char *file_path);

void send_file(const char *ftp_cmd, char *file_path);

#endif