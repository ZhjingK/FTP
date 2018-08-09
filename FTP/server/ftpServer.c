#include"ftpServer.h"
#define AUTH "auth"

//处理一个ftp事件
void ftpserver_process(int sock_ctl)
{
	//发送接受处理响应码，表示新用户服务已准备好
	send_response(sock_ctl,220);

	if (1==ftpserver_login(sock_ctl))//登录成功
	{
		//用户登录成功，回复一个230用户登录响应码
		send_response(sock_ctl,230);
	}
	//登录失败，结束本次会话
	else
	{
		send_response(sock_ctl,530);
		return;
	}

	char cmd[5];
	char arg[MAXSIZE];
	while(1)
	{
		//接收命令，解析，获取命令和参数
		int status=ftpserver_recv_cmd(sock_ctl,cmd,arg);
		//出错或服务关闭控制连接时，退出
		if((status<0)||(221==status))
		{
			break;
		}

		//命令成功,进行处理
		if(200==status)
		{
			int sock_data=ftpserver_start_data_conn(sock_ctl);
			if(sock_data<0)
			{
				close(sock_ctl);
				return;
			}

			//执行命令
			if(strcmp(cmd,"LIST")==0)
			{
				ftpserver_list(sock_data,sock_ctl);
			}
			else if(strcmp(cmd,"RETR")==0)
			{
				ftpserver_retr(sock_data,sock_ctl,arg);
			}
			else if(strcmp(cmd,"PUSH")==0)
			{
				ftpserver_push(sock_data,sock_ctl,arg);
			}
			close(sock_data);
		}
	}
}

//从控制连接中接收命令
int ftpserver_recv_cmd(int sock_ctl,char* cmd,char* arg)
{
	int status=200;
	char buf[MAXSIZE];
	memset(buf,0,MAXSIZE);
	memset(cmd,0,5);
	memset(arg,0,MAXSIZE);
	if(-1==recv_data(sock_ctl,buf,MAXSIZE))
	{
		return -1;
	}
	strncpy(cmd,buf,4);
	strcpy(arg,buf+5);
	if(strcmp(cmd,"QUIT")==0)
	{
		//服务关闭控制连接
		status=221;
	}
	else if((strcmp(cmd,"USER")==0)||(strcmp(cmd,"PASS")==0)||\
			(strcmp(cmd,"LIST")==0)||(strcmp(cmd,"RETR")==0)||(strcmp(cmd,"PUSH")==0))
	{
		//命令成功
		status=200;
	}
	else
	{
		//命令未实现
		status=502;
	}

	//发送状态
	send_response(sock_ctl,status);
	return status;
}

//用户登录
int ftpserver_login(int sock_ctl)
{
	char buf[MAXSIZE];
	char user[MAXSIZE];
	char pass[MAXSIZE];
	memset(buf,0,MAXSIZE);
	memset(user,0,MAXSIZE);
	memset(pass,0,MAXSIZE);

	if(recv_data(sock_ctl,buf,sizeof(buf))<0)
	{
		return -1;
	}

	int i=5;//Name字段后面跟的是真正的用户名
	int j=0;
	while(buf[i]!=0)//将用户名保存起来
	{
		user[j++]=buf[i++];
	}

	send_response(sock_ctl,331);//通知用户输入密码

	memset(buf,0,MAXSIZE);
	if(recv_data(sock_ctl,buf,sizeof(buf))<0)//获取密码
	{
		return -1;
	}

	i=5;//pass字段后面跟的是真正的密码
	j=0;
	while(buf[i]!=0)//将密码保存起来
	{
		pass[j++]=buf[i++];
	}
	int ret=ftpserver_check_user(user,pass);//验证用户名和密码
	return ret;
}

//检测用户的用户名和密码
int ftpserver_check_user(const char* user,const char* pass)
{
	FILE* fd=fopen(AUTH,"r");
	if(NULL==fd)
	{
		return -1;
	}

	char username[MAXSIZE];
	char password[MAXSIZE];
	char buf[MAXSIZE];
	char* line=NULL;
	int len=0;
	int auth=-1;
	//getline函数按行读取文件
	while(-1!=getline(&line,&len,fd))
	{
		memset(buf,0,MAXSIZE);
		strcpy(buf,line);

		char* post=strtok(buf," ");//分割出用户名
		strcpy(username,post);

		if(NULL!=post)
		{
			char* post=strtok(NULL," ");//分割出密码
			strcpy(password,post);
		}
		trimstr(password,(int)strlen(password));//去除字符串中的空格和换行

		if((strcmp(user,username)==0)&&(strcmp(pass,password)==0))
		{
			auth=1;
			break;
		}
	}
	free(line);
	fclose(fd);
	return auth;
}

//创建一个数据连接
int ftpserver_start_data_conn(int sock_ctl)
{
	int wait;
	if(recv(sock_ctl,&wait,sizeof(wait),0)<0)
	{
		return -1;
	}

	char buf[1024];
	struct sockaddr_in client_addr;
	socklen_t len=sizeof(client_addr);
	//getpeername函数用于获取与某个套接字关联的外地协议地址
	getpeername(sock_ctl,(struct sockaddr*)&client_addr,&len);
	//inet_ntop函数将二进制数值格式转化为点分十进制的ip地址格式
	inet_ntop(AF_INET,&client_addr.sin_addr,buf,sizeof(buf));

	int sock_data=socket_connect(buf,CLIENT_PORT);
	if(sock_data<0)
	{
		return -1;
	}
	return sock_data;
}

//处理客户端发送来的ls命令
int ftpserver_list(int sock_data,int sock_ctl)
{
	//system函数先fork一个子进程，在这个子进程中调用/bin/sh -c来执行函数参数指定的命令
	int ret=system("ls -l ftp > temp.txt");
	if(ret<0)
	{
		return -1;
	}

	int fd=open("temp.txt",O_RDONLY);
	send_response(sock_ctl,1);//准备发送数据
	struct stat st;
	stat("temp.txt",&st);
	size_t size=st.st_size;

	sendfile(sock_data,fd,NULL,size);
	close(fd);

	send_response(sock_ctl,226);//发送应答码，表示关闭文件连接，请求文件操作成功
	return 0;
}

//下载文件
void ftpserver_retr(int sock_data,int sock_ctl,char* filename)
{
	char name[260];
	memset(name,0,sizeof(name));
	strcpy(name,"ftp/");
	strcat(name,filename);
	int fd=open(name,O_RDONLY);
	if(fd<0)
	{
		send_response(sock_ctl,550);//发送错误码，未执行请求的操作
	}
	else
	{
		//文件状态良好，打开数据连接
		send_response(sock_ctl,150);
		struct stat st;
		stat(name,&st);
		size_t size=st.st_size;
		sendfile(sock_data,fd,NULL,size);
		send_response(sock_ctl,226);//文件传输完成
		close(fd);
	}
}

//上传文件
void ftpserver_push(int sock_data,int sock_ctl,char* filename)
{
	int ack;
	if(recv(sock_ctl,&ack,sizeof(ack),0)<0)
	{
		send_response(sock_ctl,502);//命令未实现,执行失败
		return;
	}

	int status=ntohl(ack);
	if(553==status)
	{
		//客户端上传文件失败
		send_response(sock_ctl,553);
		return;
	}

	char name[260];
	memset(name,0,sizeof(name));
	strcpy(name,"ftp/");
	strcat(name,filename);
	int fd=open(name,O_CREAT|O_WRONLY,0664);
	if(fd<0)
	{
		send_response(sock_ctl,502);//命令执行失败
		return;
	}

	while(1)
	{
		char data[MAXSIZE];
		memset(data,0,sizeof(data));
		ssize_t s=recv(sock_data,data,sizeof(data),0);
		if(s<=0)
		{
			if(s<0)
			{
				send_response(sock_ctl,502);//命令执行失败
			}
			else
			{
				send_response(sock_ctl,226);//命令执行成功
			}
			break;
		}
		write(fd,data,s);
	}
	close(fd);
}
