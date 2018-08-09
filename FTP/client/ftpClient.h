#ifndef __FTP_CLIENT_H__
#define __FTP_CLIENT_H__

#include"../common/common.h"

struct command
{
	//形如：LIST <name> 或 响应码 <参数>
	char code[5];
	char arg[256];
};

int read_reply(int sock_ctl);//读取服务器的回复

void print_reply(int status);//打印回复信息

int ftpclient_read_cmd(char* buf,size_t size,struct command *cmd);//读取客户端输入放入命令

int ftpclient_get(int sock_data,char* filename);//下载文件

int ftpclient_open_conn(int sock_ctl);//打开数据连接

int ftpclient_list(int sock_ctl,int sock_data);//处理list命令

int ftpclient_send_cmd(int sock_ctl,struct command* cmd);//发送命令

int ftpclient_login(int sock_ctl);

int ftpclient_put(int sock_data,char* filename);

#endif //__FTP_CLIENT_H__
