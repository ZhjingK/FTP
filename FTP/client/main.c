#include"ftpClient.h"

void Usage(const char* filename)
{
	printf("Usage: %s server_ip server_port\n",filename);
}

int main(int argc,char* argv[])
{
	if(argc!=3)
	{
		Usage(argv[0]);
		return -1;
	}

	//返回的是已经连接上指定ip和port的sock,控制连接
	int sock_ctl=socket_connect(argv[1],atoi(argv[2]));
	if(sock_ctl<0)
	{
		printf("connection failed\n");
		return -1;
	}

	printf("Connectd to %s.\n",argv[1]);
	print_reply(read_reply(sock_ctl));//读取服务器的回复并打印
	//验证身份信息
	if(ftpclient_login(sock_ctl)<0)
	{
		return -1;
	}

	while(1)
	{
		char buf[MAXSIZE];
		memset(buf,0,sizeof(buf));
		struct command cmd;
		memset(&cmd,0,sizeof(cmd));
		
		//读取客户端输入的命令
		if(ftpclient_read_cmd(buf,sizeof(buf),&cmd)<0)
		{
			printf("Invalid command\n");
			continue;
		}

		//将命令发送到服务器
		if(send(sock_ctl,buf,strlen(buf),0)<0)
		{
			break;
		}

		//读取服务器端的返回码
		int status=read_reply(sock_ctl);
		if(status==221)//退出命令
		{
			print_reply(221);//提示用户退出
			break;
		}

		if(status==502)//命令未实现(非法命令)
		{
			printf("%d Invaild commond.\n",status);
		}
		else //status is 200 命令成功
		{
			//创建一个数据连接
			int sock_data=ftpclient_open_conn(sock_ctl);
			if(sock_data<0)
			{
				perror("Error open data conect failed\n");
				break;
			}
			
			//对list命令进行处理
			//LIST命令：如果指定了文件或目录，返回其信息；否则返回当前工作目录的信息
			if(strcmp(cmd.code,"LIST")==0)
			{
				ftpclient_list(sock_ctl,sock_data);
				close(sock_data);
			}

			//处理retr命令
			//PETR命令：传输文件副本
			else if(strcmp(cmd.code,"RETR")==0)
			{
				//判断服务器文件是否正常
				if(read_reply(sock_ctl)==550)
				{
					print_reply(550);
					close(sock_data);
					continue;
				}
				
				//下载文件
				ftpclient_get(sock_data,cmd.arg);
				print_reply(read_reply(sock_ctl));
				close(sock_data);
			}

			//上传文件
			else if(strcmp(cmd.code,"PUSH")==0)
			{
				if(ftpclient_put(sock_data,cmd.arg)<0)//告诉服务器文件上传失败
				{
					send_response(sock_ctl,553);
				}
				else
				{
					send_response(sock_ctl,200);
				}

				close(sock_data);
				print_reply(read_reply(sock_ctl));//打印服务器的响应
			}
		}
	}

	close(sock_ctl);
	return 0;
}

