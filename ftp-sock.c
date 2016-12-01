#include"ftp-sock.h"

static int create_event_thread(const char *buf)
{
	int error_flag = 0;

	hEvent = CreateEvent(NULL, TRUE, FALSE, "thread_event_");
	if (hEvent == NULL)
	{
		printf("create event error\n");
		return -1;
	}
	set_port();
	hProc = _beginthread(func_ptr[transfer_mode], 0, NULL);
	if (hProc == -1)
	{
		printf("create thread error\n");
		goto th_end;
	}
	if (WaitForSingleObject(hEvent, 4000) != WAIT_OBJECT_0)
	{
		printf("time out\n");
		TerminateThread(hProc, 0x0);
		goto th_end;
	}
	ResetEvent(hEvent);
	if (transfer_mode == _PASV_&&data_sk == -1)
	{
		printf("data connect error\n");
		error_flag = -1;
		goto th_end;
	}
	if (transfer_mode == _PORT_&&listen_sk == -1)
	{
		printf("data connect error\n");
		error_flag = -1;
		goto th_end;
	}
	if (request_ftp_server(ctrl_sk, buf) == -1)
	{
		error_flag = -1;
		if (transfer_mode == _PASV_)
		{
			closesocket(data_sk);
			goto th_end;
		}
		if (transfer_mode == _PORT_)
		{
			SetEvent(hEvent);
			ResumeThread(hProc);
		}
	}
	ResumeThread(hProc);

th_end:
	if (error_flag == -1) CloseHandle(hEvent);
	return error_flag;
}

static char *get_file_name(char *path)
{
	char *tmp = NULL;

	tmp = strchr(path, '/');
	if (tmp == NULL)
		return path;
	tmp++;
	return tmp;
}

static void pasv_sk_th(void *p)
{
	data_sk = connect_server(server_ip_addr, server_data_port);
	SetEvent(hEvent);
	SuspendThread(hProc);
	SetEvent(hEvent);
}

static int send_file_(sock sk, const char *path)
{
	int ret = 0;
	char send_buf[_K_] = { NULL };
	FILE *fp = NULL;

	fp = fopen(path, "rb");
	if (fp == NULL)
	{
		printf("open file error!\n");
		return -1;
	}

	while (1)
	{//send data}
		fread(send_buf, _K_, 1, fp);
		ret = send(sk, send_buf, (int)strlen(send_buf), 0);
		if (ret == SOCKET_ERROR)
		{
			printf("send error!\n");
			return -1;
		}
		memset(send_buf, 0x0, strlen(send_buf));
		if (feof(fp))
			break;
	}
	fclose(fp);
	return 1;
}

void send_file(const char *ftp_cmd, char *file_path)
{
	char buf[512] = { NULL }, *file_name = NULL;

	strcpy(buf, ftp_cmd);
	file_name = get_file_name(file_path);
	strcat(buf, file_name);
	if (create_event_thread(buf) == -1) return;
	WaitForSingleObject(hEvent, 3000);
	send_file_(data_sk, file_path);

	closesocket(data_sk);
	recv_msg(ctrl_sk);
	if (transfer_mode == _PORT_) closesocket(listen_sk);
}

static unsigned int rand_port()
{
	time_t tm;

	//Generate a random port
	time(&tm);
	srand(tm);
	return (unsigned int)(rand() % 64512 + 1023);
}

static int recv_file_(sock sk, const char *path)
{
	char *recv_buf = NULL;
	int ret = 0;
	FILE *fp = NULL;

	fp = fopen(path, "wb");
	if (fp == NULL)
	{
		printf("open file error!\n");
		return -1;
	}

	recv_buf = (char*)malloc(_M_ + 1);
	memset(recv_buf, 0x0, _M_ + 1);
	do{
		ret = recv(data_sk, recv_buf, _M_, 0);
		if (ret <= 0)
		{
			printf("recv message error!\n");
			fclose(fp);
			return -1;
		}
		fputs(recv_buf, fp);
		memset(recv_buf, 0x0, strlen(recv_buf));
	} while (ret == _M_);
	free(recv_buf);
	fclose(fp);

	return 1;
}

void recv_file(const char *ftp_cmd, char *file_path)
{
	char buf[512] = { NULL };

	strcpy(buf, ftp_cmd);
	strcat(buf, file_path);
	if (create_event_thread(buf) == -1) return;
	WaitForSingleObject(hEvent, 3000);
	recv_file_(data_sk, file_path);
	recv_msg(ctrl_sk);
	CloseHandle(hEvent);
	if (transfer_mode == _PORT_) closesocket(listen_sk);
	closesocket(data_sk);
}

static void set_port()//send or recv
{
	char cmd[30] = { NULL };
	int a, b, c, d;

	if (transfer_mode == _PASV_)
	{
		request_ftp_server(ctrl_sk,"pasv");
		return;
	}

	if (transfer_mode == _PORT_)
	{
		local_data_port = rand_port();
		sscanf(local_ip_addr, "%d.%d.%d.%d", &a, &b, &c, &d);
		sprintf(cmd, "port %d,%d,%d,%d,%d,%d\r\n", a, b, c, d, local_data_port / 256, local_data_port % 256);
		request_ftp_server(ctrl_sk, cmd);
		return;
	}
}

static void port_sk_th(void *p)
{
	struct sockaddr_in sk_addr;

	listen_sk = socket(AF_INET, SOCK_STREAM, 0);
	sk_addr.sin_family = AF_INET;
	sk_addr.sin_addr.s_addr = INADDR_ANY;
	sk_addr.sin_port = htons(local_data_port);
	//memset(s_addr_c.sin_zero, 0x0, 8);
	if (listen_sk == -1) return;
	if (bind(listen_sk, (struct sockaddr*)&sk_addr, sizeof(struct sockaddr)) == -1)
	{
		printf("bind error\n");
		closesocket(listen_sk);
		listen_sk = -1;
		return;
	}
	if (listen(listen_sk, 5) != 0)
	{
		closesocket(listen_sk);
		listen_sk = -1;
	}
	SetEvent(hEvent);
	SuspendThread(hProc);
	if (WaitForSingleObject(hEvent, 2000) == WAIT_OBJECT_0)
	{
		closesocket(listen_sk);
		return;
	}
	data_sk = accept(listen_sk, NULL, NULL);
	SetEvent(hEvent);
}

static int recv_msg(sock sk)
{
	char recv_buf[_K_ + 1] = { NULL };
	int ret, count = 0, mem_b = 1;
	char *msg_buf = NULL, *mem_tmp = NULL;
	int a, b;

	msg_buf = (char*)malloc(_M_ + 1);
	memset(msg_buf, 0x0, _M_ + 1);

	do{
		ret = recv(sk, recv_buf, _K_, 0);
		if (ret <= 0)
		{
			printf("recv message error!\n");
			return -1;
		}
		strcat(msg_buf, recv_buf);
		memset(recv_buf, 0x0, _K_ + 1);
		count++;
		if (count>1023)
		{
			if (mem_b >= 30)
				return -1;
			mem_b++;
			mem_tmp = msg_buf;
			msg_buf = (char*)malloc(_M_*mem_b + 1);
			memset(msg_buf, 0x0, _M_*mem_b + 1);
			strcpy(msg_buf, mem_tmp);
			free(mem_tmp);
			count = 0;
		}
	} while (ret == _K_);

	ret = 0;
	if (sk == ctrl_sk)
	{
		//wrong list
		for (count = 0; count < sizeof(stat_code) / sizeof(char*); count++)
		{
			if (strncmp(msg_buf, stat_code[count], 3) == 0)
				ret = -1;
		}
	}

	if (transfer_mode == _PASV_)
	{
		sscanf(msg_buf, "227 Entering Passive Mode (%*d,%*d,%*d,%*d,%d,%d).", &a, &b);
		server_data_port = a * 256 + b;
	}
	printf("%s\n", msg_buf);

	return ret;
}

void ctrl_ftp(char *ftp_cmd)
{
	if (create_event_thread(ftp_cmd) == -1) return;
	WaitForSingleObject(hEvent, 3000);
	recv_msg(data_sk);
	recv_msg(ctrl_sk);
	CloseHandle(hEvent);
	if (transfer_mode == _PORT_) closesocket(listen_sk);
	closesocket(data_sk);
}

int request_ftp_server(sock sk, const char *cmd)
{
	int ret;
	char *send_buf;

	send_buf = (char*)malloc(strlen(cmd) + 3);
	memset(send_buf, 0x0, strlen(cmd) + 3);
	sprintf(send_buf, "%s\r\n", cmd);

	ret = send(sk, send_buf, (int)strlen(send_buf), 0);
	if (ret == SOCKET_ERROR)
	{
		printf("send error!\n");
		return -1;
	}

	free(send_buf);

	if (recv_msg(sk) == -1) return -1;

	return 1;
}

int connect_ftp_server(char *ip_addr, u_short port)
{
	char *ip_list[10];
	struct sockaddr_in local_ip;
	int len;

	ctrl_sk = connect_server(ip_addr, port);
	if (ctrl_sk == (sock)-1)
	{
		printf("fail to connect ftp server!\n");
		ctrl_sk = 0;
		return 0;
	}
	if (!hostname2ip(ip_addr, ip_list, sizeof(ip_list) / sizeof(char*)))
		return -1;
	//copy server ip_addr
	len = sizeof(struct sockaddr);
	memset(server_ip_addr, 0x0, sizeof(server_ip_addr));
	strcpy(server_ip_addr, ip_list[0]);
	if (getsockname(ctrl_sk, (struct sockaddr*)&local_ip, (socklen_t*)&len) == -1)
		return -1;
	//copy local ip_addr
	memset(local_ip_addr, 0x0, sizeof(local_ip_addr));
	sprintf(local_ip_addr, "%d.%d.%d.%d", local_ip.sin_addr.s_addr & 0x000000FF, (local_ip.sin_addr.s_addr & 0x0000FF00) >> 8, (local_ip.sin_addr.s_addr & 0x00FF0000) >> 16, (local_ip.sin_addr.s_addr & 0xFF000000) >> 24);
	Sleep(1000);
	recv_msg(ctrl_sk);

	return 1;
}

static sock connect_server(char *host, u_short port)
{
	char *ip_addr, *ip_list[10] = { NULL };
	sock sock_ftp_s;
	struct sockaddr_in s_addr_c;
	//
#ifdef _WIN32
	WSADATA ws;

	if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
	{
		printf("Init Windows Socket Failed!\n");
		return -1;
	}
#endif

	if (inet_addr(host) == INADDR_NONE)
	{
		if (hostname2ip(host, ip_list, sizeof(ip_list) / sizeof(char*)))
			ip_addr = ip_list[0];
		else{
			printf("Invalid ip address!\n");
			return -1;
		}
	}
	else ip_addr = host;

	if (port > 65535)
	{
		printf("Invalid port!\n");
		return -1;
	}

	//create socket
	sock_ftp_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

#ifdef __linux__
	if (sock_ftp_s)
	{
		printf("Create Socket ERROR!\n");
		return -1;
	}
#endif

#ifdef _WIN32
	if (sock_ftp_s == INVALID_SOCKET)
	{
		printf("Create Socket ERROR!\n");
		return -1;
	}
#endif

	s_addr_c.sin_family = AF_INET;
	if (inet_pton(AF_INET, ip_addr, &s_addr_c.sin_addr.s_addr) <= 0)
	{
		printf("inet_pton error!\n");
		return -1;
	}
	s_addr_c.sin_port = htons(port);
	memset(s_addr_c.sin_zero, 0x0, 8);

	//bind sock
	if (connect(sock_ftp_s, (struct sockaddr*)&s_addr_c, sizeof(s_addr_c))<0)
	{
		printf("connect error!\n");
		return -1;
	}

	return sock_ftp_s;
}

static int hostname2ip(char *Target, char **IpList, int ip_count)
{
	struct hostent *pHost;
	int i = 0;

	pHost = gethostbyname(Target);
	if (pHost != NULL)
	{
		while (*pHost->h_addr_list != NULL)
		{
			IpList[i] = inet_ntoa(*(struct in_addr *)*pHost->h_addr_list);
			*pHost->h_addr_list++;
			i++;
			if (i > ip_count) return 1;
		}
		return 1;
	}
	else return 0;
}