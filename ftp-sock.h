#ifndef _FTP_SOCK_
#define _FTP_SOCK_

#include"ftp-sock-out.h"

char server_ip_addr[16], local_ip_addr[16];
sock ctrl_sk = 0, listen_sk = 0, data_sk = 0;
int transfer_mode = _PORT_;

int server_data_port, local_data_port;
const char *stat_code[] = { "202", "421", "425", "450", "451", "452", "500", "501", "502", "503", "504", "530", "532", "550", "553" };
HANDLE hEvent, hProc;

static char *get_file_name(char *path);

static void port_sk_th(void*);

static int send_file_(sock sk, const char *path);

static unsigned int rand_port();

static int recv_file_(sock sk, const char *path);

static void set_port();

static void pasv_sk_th(void*);

static int recv_msg(sock sk);

static sock connect_server(char *host, u_short port);

static int create_event_thread(const char *buf);

static int hostname2ip(char *Target, char **IpList, int ip_count);

static void(*func_ptr[])(void*) = { port_sk_th, pasv_sk_th };
#endif