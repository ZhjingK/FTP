#include"ftpServer.h"
#include<pthread.h>

void Usage(const char* filename)
{
	printf("Usage: %s server_ip server_port\n",filename);
}

void* handler_msg(void *arg)
{
	//使线程处于分离状态，在线程退出时资源会自动释放，不会调用pthread_join函数释放资源
	pthread_detach(pthread_self());
	int sock=(int)arg;
	ftpserver_process(sock);
	close(sock);
	return NULL;
}

int main(int argc,char* argv[])
{
	if(argc!=3)
	{
		Usage(argv[0]);
		return -1;
	}
	
	int sock=socket_create(argv[1],atoi(argv[2]));

	while(1)
	{
		int connf=socket_accept(sock);
		if(connf<0)
		{
			continue;
		}
		pthread_t tid;
		pthread_create(&tid,NULL,handler_msg,(void*)connf);
	}
	close(sock);
	return 0;
}
