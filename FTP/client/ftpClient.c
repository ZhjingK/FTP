#include"ftpClient.h"

//读取服务器的回复
int read_reply(int sock_ctl)
{
	int status=0;
	if(recv(sock_ctl,&status,sizeof(status),0)<0)
	{
		printf("client:error reading message from server.\n");
		return -1;
	}

	return ntohl(status);
}

//打印回复信息
void print_reply(int status)
{
	switch(status)
	{
		//服务就绪
		case 220:
			printf("200 welcome! server ready.\n");
			break;

		//退出网络	
		case 221:
			printf("221 bye!\n");
			break;

		//结束数据连接
		case 226:
			printf("226 close data connection.requested fileaction successful.\n");
			break;

		//命令没有执行
		case 502:
			printf("502 command execute failed.\n");
			break;
		
		//文件不可用
		case 550:
			printf("550 request action not taken,file unavailable.\n");
			break;

		//文件名不允许
		case 553:
			printf("553 Could not file.\n");
			break;

		//无法预知的错误
		default:
			printf("unkonwn error.\n");
			break;
	}
}

//读取客户端命令行输入的命令
int ftpclient_read_cmd(char* buf,size_t size,struct command *cmd)
{
	memset(cmd->code,0,sizeof(cmd->code));
	memset(cmd->arg,0,sizeof(cmd->arg));
	printf("ftpclient> ");//输入提示符
	fflush(stdout);
	
	read_input(buf,size);//从标准输入读取一行
	char* arg=NULL;
	
	//arg先指向输入命令空格前面的字符，再调用一次strtok函数arg指向空格后面的命令参数//

	//strtok函数用来将字符串分割成一个个片段
	//将buf中读取的内容以空格分隔
	arg=strtok(buf," ");
	 //当在buf中的字符串发现" "的分割字符时则会将该字符改为'\0'
	arg=strtok(NULL," ");//指向输入的命令所带的参数
	if(NULL!=arg)
	{
		//从arg中复制strlen(arg)个字符到cmd->arg中
		strncpy(cmd->arg,arg,strlen(arg));
	}

	//比较输入的指令
	if(strncmp(buf,"ls",4)==0)
	{
		strcpy(cmd->code,"LIST");//LIST用来获取FTP上指定目录下的文件列表信息
	}
	else if(strncmp(buf,"get",3)==0)
	{
		strcpy(cmd->code,"RETR");//下载文件
	}
	else if(strncmp(buf,"bye",3)==0)
	{
		strcpy(cmd->code,"QUIT");//关闭与服务器连接
	}
	else if(strncmp(buf,"put",3)==0)
	{
		strcpy(cmd->code,"PUSH");//上传文件？？？
	}
	else
	{
		return -1;
	}

	//将这个命令再存到buf中
	memset(buf,0,size);
	strcpy(buf,cmd->code);
	//如果命令带参数将参数追加到命令的后面
	if(NULL!=arg)
	{
		strcat(buf," ");
		strncat(buf,cmd->arg,strlen(cmd->arg));
	}
	return 0;
}

//上传文件
int ftpclient_put(int sock_data,char* filename)
{
	int fd=open(filename,O_RDONLY);
	if(fd<0)
	{
		return -1;
	}

	//描述一个Linux系统文件系统中的文件属性的结构
	struct stat st;
	if(stat(filename,&st)<0)
	{
		return -1;
	}

	sendfile(sock_data,fd,NULL,st.st_size);
	close(fd);
	return 0;

}

//下载文件
int ftpclient_get(int sock_data,char* filename)
{
	int fd=open(filename,O_CREAT|O_WRONLY,0664);
	while(1)
	{
		ssize_t s=0;
		char data[MAXSIZE];
		memset(data,0,sizeof(data));
		s=recv(sock_data,data,sizeof(data),0);
		if(s<=0)
		{
			if(s<0)
			{
				perror("recv");
			}
			break;
		}
		write(fd,data,s);
	}
	close(fd);
	return 0;
}

//打开数据连接
int ftpclient_open_conn(int sock_ctl)
{
	//ip为0.0.0.0代表本地所有ip地址
	int sock_listen=socket_create("0.0.0.0",CLIENT_PORT);//创建一个数据连接
	
	//给服务器发送一个确认，告诉服务器客户端创建好了一条数据连接
	int ack=1;
	if(send(sock_ctl,(char*)&ack,sizeof(ack),0)<0)
	{
		printf("client:ack write error:%d\n",errno);
		return -1;
	}
	int sock_data=socket_accept(sock_listen);
	close(sock_listen);
	return sock_data;
}

//处理list命令
int ftpclient_list(int sock_ctl,int sock_data)
{
	//等待服务器连接成功发送过来一个信号
	int tmp=0;
	if(recv(sock_ctl,&tmp,sizeof(tmp),0)<0)
	{
		perror("client:error reading message from server.\n");
		return -1;
	}

	//接收服务器发送过来的数据
	while(1)
	{
		char buf[MAXSIZE];
		memset(buf,0,sizeof(buf));
		ssize_t s=recv(sock_data,buf,MAXSIZE,0);
		if(s<=0)
		{
			if(s<0)
			{
				perror("recv");
			}
			break;
		}
		printf("%s",buf);
	}
	
	//等待服务器发送完成的信号
	if(recv(sock_ctl,&tmp,sizeof(tmp),0)<0)
	{
		perror("client:error reading message from server.\n");
		return -1;
	}
	return 0;
}

//从控制连接发送命令
int ftpclient_send_cmd(int sock_ctl,struct command* cmd)
{
	char buf[MAXSIZE];
	sprintf(buf,"%s %s",cmd->code,cmd->arg);
	if(send(sock_ctl,buf,strlen(buf),0)<0)
	{
		perror("error sending command to server.\n");
		return -1;
	}
	return 0;
}

//向服务器发送用户名密码进行用户信息确认
int ftpclient_login(int sock_ctl)
{
	struct command cmd;
	char user[256];
	memset(user,0,sizeof(user));

	printf("Name: ");
	fflush(stdout);
	read_input(user,256);//获取用户名

	strcpy(cmd.code,"USER");
	strcpy(cmd.arg,user);
	ftpclient_send_cmd(sock_ctl,&cmd);//发送用户名到服务器

	int wait=0;
	recv(sock_ctl,&wait,sizeof(wait),0);//接受应答码 331 要求密码

	fflush(stdout);
	//getpass函数用于从控制台输入了一行字符串关闭回显，使用于密码的输入
	//通常与crypt加密函数搭配使用
	char* pass=getpass("Password: ");//使用getpass函数获取密码

	strcpy(cmd.code,"PASS");
	strcpy(cmd.arg,pass);
	ftpclient_send_cmd(sock_ctl,&cmd);//发送密码到服务器

	int retcode=read_reply(sock_ctl);//读取服务器的回应
	switch(retcode)
	{
		case 530:
			printf("Invalid username/password.\n");//用户名/密码错误
			return -1;
		case 230:
			printf("Successful login.\n");//登录成功
			break;
		default:
			printf("error reading message from server.\n");
			return -1;
	}
	return 0;
}

