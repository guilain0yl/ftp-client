#include"ftp-assist.h"

static int exec(char *argv[], int argc)
{
	int i, port = FTP_DEFAULT_PORT;
	char buf[1024] = { NULL };

	for (i = 0; i<sizeof(cmd2ftpcmd) / sizeof(struct _CMD_2_FTPCMD_); i++)
	{
		if (stricmp(cmd2ftpcmd[i].mnem_cmd, argv[0]) == 0)
		{
			if (ctrl_sk <= 0)
			{
				printf("no connect");
				return 1;
			}
			if (cmd2ftpcmd[i].interactive)
			{
				if (cmd2ftpcmd[i].parameter)
				{
					if (argc>2 || argc == 1)
					{
						printf("usage ");
						printf(argv[0]);
						printf(" [file path]");
						return 1;
					}
					else
					{
						if (cmd2ftpcmd[i].interactive == _SEND_)
							send_file(cmd2ftpcmd[i].ftp_cmd, argv[1]);
						if (cmd2ftpcmd[i].interactive == _RECV_)
							recv_file(cmd2ftpcmd[i].ftp_cmd, argv[1]);
						return 1;
					}
				}
				else
				{
					ctrl_ftp(cmd2ftpcmd[i].ftp_cmd);
					return 1;
				}
			}
			else{
				if (cmd2ftpcmd[i].parameter)
				{
					if (argc>2)
					{
						printf("usage ");
						printf(argv[0]);
						printf(" [file path]");
						return 1;
					}
					strcpy(buf, cmd2ftpcmd[i].ftp_cmd);
					strcat(buf, argv[1]);
					request_ftp_server(ctrl_sk, buf);
				}
				else
				{
					request_ftp_server(ctrl_sk, cmd2ftpcmd[i].ftp_cmd);
					return 1;
				}
			}
		}
	}

	if (stricmp(argv[0], "rename") == 0)
	{
		if (ctrl_sk <= 0) printf("no connect\n");
		if (argc != 3)
			printf("usage: rename [old name] [new name]");
		else{
			strcpy(buf, "rnfr "); strcat(buf, argv[1]);
			request_ftp_server(ctrl_sk, buf);
			strcpy(buf, "rnto "); strcat(buf, argv[2]);
			request_ftp_server(ctrl_sk, buf);
		}
		return 1;
	}

	if (stricmp(argv[0], "port") == 0)
	{
		transfer_mode = _PORT_;
		printf("current mode: port");
		return 1;
	}

	if (stricmp(argv[0], "pasv") == 0)
	{
		transfer_mode = _PASV_;
		printf("current mode: pasv");
		return 1;
	}

	if (stricmp(argv[0], "login") == 0)
	{
		if (ctrl_sk <= 0)
			printf("no connect");
		else login_();
		return 1;
	}

	if (stricmp(argv[0], "open") == 0)
	{
		if (argc == 1 || argc>3)
			printf("usage: open <hostname or ipadder> [port]\n");
		if (argc == 3)
			port = atoi(argv[2]);
		if (connect_ftp_server(argv[1], port)==1)
			login_();
		return 1;
	}
	if (stricmp(argv[0], "close") == 0 || stricmp(argv[0], "quit") == 0)
	{
		if (ctrl_sk <= 0)
			printf("no connect");
		else{
			request_ftp_server(ctrl_sk, "quit");
			closesocket(ctrl_sk);
			ctrl_sk = 0;
			WSACleanup();
		}
		return 1;
	}
	if (stricmp(argv[0], "help") == 0 || stricmp(argv[0], "?") == 0)
	{
		show_help();
		return 1;
	}
	if (stricmp(argv[0], "bell") == 0)
	{
		bell = !bell;
		if (bell)
			printf("bell : on");
		else printf("bell : off");
		return 1;
	}
	if (stricmp(argv[0], "exit") == 0)
		return -1;

	return 0;
}

int exe_cmd(char *argv[],int argc)
{
	int ret = exec(argv, argc);
	if (ret == 0)
		printf("unknown command!");
	if (bell)
		BELL;
	if (ret == -1)
		return -1;
}

static void login_()
{
	char arg_buf[1024] = { NULL };
	char buf[512] = { NULL };

	printf("user:");
	memset(arg_buf, 0, 1024);
	scanf("%512s", &buf);
	sprintf(arg_buf, "user %s", buf);
	request_ftp_server(ctrl_sk, arg_buf);
	memset(arg_buf, 0, 1024);
	memset(buf, 0, 512);
	printf("password:");
	if (getpass(buf, 512) == -1)
	{
		printf("overflow\n"); return;
	}
	sprintf(arg_buf, "pass %s", buf);
	printf("\n");
	request_ftp_server(ctrl_sk, arg_buf);
}

static int getpass(char *buf, int buf_len)
{
	char c; int i = 0;

	while ((c = getch()) != '\r')
	{
		buf[i] = c; i++;
		if (c == 0x8) i--;
		//putchar('*');
		if (i > buf_len) return -1;
	}

	buf[i] = 0x0;

	return 1;
}

void ana_clean(char *argv[], int argc)
{
	int i;
	for (i = 0; i<argc; i++)
		free(argv[i]);

	while ((i = getchar()) != '\n'&&i != EOF);
}

int ana_cmd(char *cmd, char *argv[], int *argc)
{
	int i = 0;
	char c;
	char *sta = NULL;

	*argc = 0;

	while (!is_display_character(cmd[i]))
	{
		i++;
		if (i>strlen(cmd)) return -1;
	}
	sta = &cmd[i];
	c = sta[i];

	while (c != 0x0)
	{
		i = 0;
		while (sta[i] != ' '&&sta[i] != 0x0) i++;
		argv[*argc] = (char*)malloc(i + 1);
		memset(argv[*argc], 0x0, i + 1);
		strncpy(argv[*argc], sta, i);
		c = sta[i];
		(*argc)++;
		if (*argc>MAX_NUM_ARG) return -1;

		while (!is_display_character(sta[i]))
		{
			i++;
			if (i>strlen(sta)) return -1;;
		}
		sta = &sta[i];
	}
	return 1;
}

static int is_display_character(char c)
{
	if (c >= 0x21 && c <= 0x7e)
		return 1;
	return 0;
}


static void show_help()
{
	int i,k;

	const char *help_tip = "Command may be abbreviated. Command as:\n\n";
	printf("%s", help_tip);
	for (i = 0; i < sizeof(local_command) / sizeof(char*); i++)
		printf("%-15s", local_command[i]);
	printf("\n\r");
	for (k = 0; k < sizeof(cmd2ftpcmd) / sizeof(struct _CMD_2_FTPCMD_) / 6 + 1; k++)
	{
		for (i = 0; i < 6; i++)
		{
			printf("%-15s", cmd2ftpcmd[6 * k + i].mnem_cmd);
			if ((6 * k + i) == sizeof(cmd2ftpcmd) / sizeof(struct _CMD_2_FTPCMD_) - 1)
				return;
		}
		printf("\n\r");
	}
}