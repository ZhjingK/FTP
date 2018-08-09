#ifndef __FTP_SERVER_H__
#define __FTP_SERVER_H__

#include"../common/common.h"
#include<sys/stat.h>

void ftpserver_process(int sock_ctl);//用于处理一个ftp事件

int ftpserver_recv_cmd(int sock_ctl,char* cmd,char* arg);//从控制连接中接受命令

int ftpserver_login(int sock_ctl);//用户登录

int ftpserver_check_user(const char* user,const char* pass);//检测用户的用户名和密码

int ftpserver_start_data_conn(int sock_ctl);//创建数据连接

int ftpserver_list(int sock_data,int sock_ctl);//处理客户端发来的list命令

void ftpserver_retr(int sock_data,int sock_ctl,char* filename);// 处理文件下载

void ftpserver_push(int sock_data,int sock_ctl,char* filename);//处理文件上传

#endif //__FTP_SERVER_H__

