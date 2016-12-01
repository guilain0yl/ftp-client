#ifndef _FTP_ASSIST_
#define _FTP_ASSIST_
#include"ftp-sock-out.h"

#define _SEND_ 2
#define _RECV_ 3

#define FTP_DEFAULT_PORT 21
#define BELL printf("\a");

static int bell = 0;

static const struct _CMD_2_FTPCMD_
{
	char *mnem_cmd;
	char *ftp_cmd;
	int interactive;
	int parameter;
}cmd2ftpcmd[] = {
	"dir", "list", 1, 0,
	"ls", "list", 1, 0,
	"put", "stor ", _SEND_, 1,
	"get", "retr ", _RECV_, 1,
	"cd", "cwd ", 0, 1,
	"pwd", "pwd", 0, 0,
	"cdup", "cdup", 0, 0,
	"delete", "dele ", 0, 1,
	"rhelp", "help", 0, 0,
	"mkdir", "mkd ", 0, 1,
	"remove", "rmd ", 0, 1,
	"binary", "type I", 0, 0,
	"ascii", "type A", 0, 0,
	"stream", "stru s", 0, 0,
	"block", "stru b", 0, 0,
	"compressed", "stru c", 0, 0,
	"append", "appe ", _SEND_, 1
};

static const char *local_command[] = { "bell", "help", "?", "open", "close", "rename" };

#define MAX_NUM_ARG 512
#define stricmp _stricmp

static int is_display_character(char c);

static int getpass(char *buf, int buf_len);

int ana_cmd(char *cmd, char *argv[], int *argc);

void ana_clean(char *argv[], int argc);

static void login_();

static int exec(char *argv[], int argc);

int exe_cmd(char *argv[], int argc);

static void show_help();

#endif