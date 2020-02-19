#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

enum CMD
{
	CMD_ERROR,
	CMD_LOGIN,
	CMD_LOGINOUT,
};
struct DataHeader
{
	short cmd;
	short dataLength;
};
struct Login : public DataHeader
{
	Login()
	{
		cmd = CMD_LOGIN;
		dataLength = sizeof(Login);
	}
	char uName[32];
	char uPassword[32];
};
struct LoginResult : public DataHeader
{
	LoginResult()
	{
		cmd = CMD_LOGIN;
		dataLength = sizeof(LoginResult);
		result = false;
	}
	bool result;
};
struct Loginout : public DataHeader
{
	Loginout()
	{
		cmd = CMD_LOGINOUT;
		dataLength = sizeof(Loginout);
	}
	char uName[32];
};
struct LoginoutResult : public DataHeader
{
	LoginoutResult()
	{
		cmd = CMD_LOGINOUT;
		dataLength = sizeof(LoginoutResult);
		result = false;
	}
	bool result;
};

vector<SOCKET> g_clients;

int DoProcessor(SOCKET _cSock)
{
	//缓冲区
	char arrayRecv[1024] = {};
	//5. 接受客户端数据
	int nLen = recv(_cSock, arrayRecv, sizeof(DataHeader), 0);
	DataHeader* header = (DataHeader*)arrayRecv;
	if (nLen <= 0)
	{
		printf("客户端已退出，任务结束！\n");
		return -1;
	}
	printf("收到命令：%d 数据长度：%d\n", header->cmd, header->dataLength);

	//6. 处理请求并发送给客户端
	printf("Handling Client...\n");
	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		Login login = {};
		recv(_cSock, (char*)&login + sizeof(DataHeader), sizeof(Login) - sizeof(DataHeader), 0);
		printf("CMD_LOGIN name: %s ; password: %s\n", login.uName, login.uPassword);
		LoginResult ret;
		ret.result = true;
		send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
	}
	break;
	case CMD_LOGINOUT:
	{
		Loginout loginout = {};
		recv(_cSock, (char*)&loginout + sizeof(DataHeader), sizeof(Loginout) - sizeof(DataHeader), 0);
		printf("CMD_LOGIN name: %s\n", loginout.uName);
		LoginoutResult ret;
		ret.result = true;
		send(_cSock, (char*)&ret, sizeof(LoginoutResult), 0);
	}
	break;
	default:
	{
		DataHeader header = {};
		header.cmd = CMD_ERROR;
		header.dataLength = 0;
		send(_cSock, (char*)&header, sizeof(DataHeader), 0);
	}
	break;
	}
	return 1;
}

int main() {
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	//启动Windows socket环境
	WSAStartup(ver, &dat);
	//1. 建立套接字socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//2. bind 
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);//host to net unsigned short
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (SOCKET_ERROR == (bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in))))
	{
		printf("Bind Error!\n");
		return 0;
	}
	//3. listen,第二个参数表示默认多少人能连接
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("Listen Error!\n");
		return 0;
	}
	while(true)
	{
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;
			
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);

		for (size_t i = g_clients.size()-1; i > 0; i--)
		{
			FD_SET(g_clients[i], &fdRead);
		}
		//nfds 是一个整数值，是指fd_set集合中所有描述符（socket）的范围，而不是数量
		//即是所有文件描述符最大值+1（Windows平台下已处理，可写0）
		int ret = select(_sock+1, &fdRead, &fdWrite, &fdExp, NULL);
		if (ret < 0)
		{
			printf("select失败，任务结束\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			//4. accept 等待接受客户端连接
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(clientAddr);
			SOCKET _cSock = INVALID_SOCKET;
			_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
			if (INVALID_SOCKET == _cSock)
			{
				printf("Accept Error!\n");
			}
			printf("新客户端加入 IP: %s\n", (inet_ntoa)(clientAddr.sin_addr));
			g_clients.push_back(_cSock);
		}
		for (size_t i = 0; i < fdRead.fd_count; i++)
		{
			if (-1 == DoProcessor(fdRead.fd_array[i]))
			{
				auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[i]);
				if (iter != g_clients.end())
				{
					g_clients.erase(iter);
				}
			}
		}
	}
	for (size_t i = g_clients.size() - 1; i > 0; i--)
	{
		closesocket(g_clients[i]);
	}
	//7. 关闭套接字
	closesocket(_sock);
	//清除Windows socket环境
	WSACleanup();
	getchar();
	return 0;
}