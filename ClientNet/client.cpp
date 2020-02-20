#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <windows.h>
	#include <WinSock2.h>
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <string.h>
	
	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif // _WIN32

#include <stdio.h>
#include <thread>
#include "defines.h"

#pragma warning(disable:4996)


class CNetClient
{
private:
	SOCKET _sock;
	fd_set fdMain;	//创建一个用来装socket的结构体
public:
	CNetClient()
	{
		_sock = INVALID_SOCKET;
	}
	virtual ~CNetClient()
	{
		Close();
	}
	//初始化socket
	int InitSocket()
	{
		#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		//启动Windows socket环境
		WSAStartup(ver, &dat);
		#endif // _WIN32
		if (INVALID_SOCKET != _sock)
		{
			printf("关闭旧连接<socket = %d>！\n", _sock);
			Close();
		}
		//1. 建立套接字socket
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("Socket error!\n");
			getchar();
			Close();
			return 0;
		}
		printf("Socket Success!\n");
		return 1;
	}
	//连接到服务器
	bool Connect(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}
		//2. connect服务器
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
		#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		#else
		_sin.sin_addr.s_addr = inet_addr(ip);
		#endif // _WIN32

		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("Connect Error!\n");
			getchar();
			return 0;
		}
		FD_ZERO(&fdMain);//将你的套节字集合清空
		FD_SET(_sock, &fdMain);//加入你感兴趣的套节字到集合,这里是一个读数据的套节字s
		printf("Connect Server Success!\n");
		return 1;
	}
	//关闭socket
	void Close()
	{
		FD_ZERO(&fdMain);//将你的套节字集合清空

		#ifdef _WIN32
		//7. 关闭套接字
		closesocket(_sock);
		//清除Windows socket环境
		WSACleanup();
		#else
		close(_sock);
		#endif // _WIN32

		getchar();
	}
	//发送数据
	bool OnRun()
	{
		if ( !IsRun())
		{
			return false;
		}
		fd_set fdRead = fdMain;
		fd_set fdWrite = fdMain;
		fd_set fdExp = fdMain;
		timeval st = { 1, 0 };
		int ret = select(_sock + 1, &fdRead, 0, 0, &st);
		if (ret < 0)
		{
			printf("select程序结束\n");
			return false;
		}
		else if (0 == ret)
		{
			//printf("空闲处理其他业务！\n");
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			if (0 == RecvData(_sock))
			{
				printf("select任务结束！\n");
				return false;
			}
		}
		return true;
	}

	//判断当前sock是否正常
	bool IsRun()
	{
		return INVALID_SOCKET != _sock && g_bRun;
	}

	//接受数据
	bool RecvData(SOCKET _sock)
	{
		//缓冲区
		char arrayRecv[1024] = {};
		//5. 接受客户端数据
		int nLen = recv(_sock, arrayRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)arrayRecv;
		if (nLen <= 0)
		{
			printf("与服务器断开连接，任务结束！\n");
			return false;
		}
		printf("收到<Socket = %d> 命令：%d 数据长度：%d\n", _sock, header->cmd, header->dataLength);
		recv(_sock, arrayRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(header);
		return true;
	}

	//处理网络消息
	void OnNetMsg(DataHeader* header)
	{
		//6. 处理请求
		printf("Handling Client...\n");
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			LoginResult* loginRet = (LoginResult*)header;
			printf("接收服务器发送的信息：%d\n", loginRet->result);
		}
		break;
		case CMD_LOGINOUT:
		{
			LoginoutResult* loginoutRet = (LoginoutResult*)header;
			printf("接收服务器发送的信息：%d\n", loginoutRet->result);
		}
		break;
		default:
		{
			printf("服务器发送数据Error!\n");
		}
		break;
		}
	}

	int SendData(DataHeader * header)
	{
		if (!(IsRun() && header))
		{
			return SOCKET_ERROR;
		}
		return send(_sock, (char*)header, header->dataLength, 0);
	}
};

void CmdThread(CNetClient * client)
{
	while (g_bRun)
	{
		//3. 输入请求命令
		char cmdBuf[128] = {};
		printf("Handling...\n");
		scanf("%s", cmdBuf);
		//4. 处理请求命令
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("收到退出命令exit,程序退出！\n");
			getchar();
			return;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login;
			strcpy(login.uName, "name");
			strcpy(login.uPassword, "mima");
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "loginout"))
		{
			Loginout loginout;
			strcpy(loginout.uName, "name");
			client->SendData(&loginout);
		}
		else
		{
			DataHeader dh = { CMD_ERROR, 0 };
			client->SendData(&dh);
		}
	}
}

int main() 
{
	CNetClient client;
	client.InitSocket();
	bool ret = client.Connect("127.0.0.1", 7777);
	if (false == ret)
	{
		return false;
	}
	//启动输入线程
	std::thread  t1(CmdThread, &client);
	t1.detach();

	while (client.IsRun())
	{
		if (!client.OnRun())
		{
			break;
		}
	}
	client.Close();
	return true;
}
