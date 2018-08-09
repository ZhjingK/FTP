#include"common.h"

//创建一个监听套接字
int socket_create(const char* ip,const int port)
{
	if(NULL==ip || (port<0))
	{
		return -1;
	}

	int sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock<0)
	{
		return -1;
	}

	//防止服务器首先断开连接之后进入TIME_WAIT状态，无法立即重启
	int opt=1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	struct sockaddr_in local;
	local.sin_family=AF_INET;
	local.sin_port=htons(port);
	local.sin_addr.s_addr=inet_addr(ip);

	if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0)
	{
		return -1;
	}

	if(listen(sock,5)<0)
	{
		return -1;
	}

	return sock;
}

//接受连接
int socket_accept(int sock)
{
	struct sockaddr_in peer;
	socklen_t len=sizeof(peer);

	//返回一个新的socket文件描述符用于数据的传输
	int connfd=accept(sock,(struct sockaddr*)&peer,&len);
	if(connfd<0)
	{
		return -1;
	}
	return connfd;

}

//连接远端主机
int socket_connect(const char* ip,const int port)
{
	if(NULL==ip || (port<0))
	{
		return -1;
	}

	int sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock<0)
	{
		return -1;
	}

	struct sockaddr_in peer;
	peer.sin_family=AF_INET;
	peer.sin_port=htons(port);
	peer.sin_addr.s_addr=inet_addr(ip);

	if(connect(sock,(struct sockaddr*)&peer,sizeof(peer))<0)
	{
		return -1;
	}

	return sock;
}

//从sock读取数据
int recv_data(int sock,char* buf,int bufsize)
{
	memset(buf,0,bufsize);
	ssize_t s=recv(sock,buf,bufsize,0);
	if(s<=0)
	{
		return -1;
	}
	return s;
}

//发送响应码到sock
int send_response(int sock,int code)
{
	int stat_code=htonl(code);
	if(send(sock,&stat_code,sizeof(stat_code),0)<0)
	{
		return -1;
	}
	return 0;

}

//从标准输入读取一行
void read_input(char* buf,int buffsize)
{
	memset(buf,0,buffsize);
	//fgets函数从文件中读取字符串，每次只读取一行
	if(NULL!=fgets(buf,buffsize,stdin))
	{
		//strchr函数在buf所指向的字符串中搜索第一次出现字符'\n'的位置
		char* n=strchr(buf,'\n');
		if(n)
			*n='\0';
	}
}

//去除字符串中的空白和换行
void trimstr(char* str,int n)
{
	int i=0;
	for(i=0;i<n;i++)
	{
		if(isspace(str[i]) || (str[i]=='\0'))
		{
			str[i]=0;
		}
	}
}










