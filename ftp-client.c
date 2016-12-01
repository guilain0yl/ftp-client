

#include"ftp-sock-out.h"
#include"ftp-assist.h"
//critical section


int main()
{
	int argc = 0;
	char *argv[MAX_NUM_ARG], c;
	char arg_buf[_K_];

next:
	memset(arg_buf, 0x0, sizeof(arg_buf));
	printf("ftp>");
	if ((c = getc(stdin)) == '\n')
	{
		printf("\r"); goto next;
	}
	else ungetc(c, stdin);
	scanf("%1024[^\n]", &arg_buf);
	ana_cmd(arg_buf, argv, &argc);//

	//exec
	if (exe_cmd(argv, argc) == -1)
		goto end;

	printf("\n");
	ana_clean(argv, argc);//
	goto next;
end:;
}