#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

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
	//4. accept 等待接受客户端连接
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(clientAddr);
	SOCKET _cSock = INVALID_SOCKET;

	while (true)
	{
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
		if (INVALID_SOCKET == _cSock)
		{
			printf("Accept Error!\n");
		}
		printf("新客户端加入 IP: %s\n", (inet_ntoa)(clientAddr.sin_addr));

		while(true)
		{
			DataHeader header = {};
			//5. 接受客户端数据
			int nLen = recv(_cSock, (char*)&header, sizeof(DataHeader), 0);
			if (nLen <= 0)
			{
				printf("客户端已退出，任务结束！\n");
				break;
			}
			printf("收到命令：%d 数据长度：%d\n", header.cmd, header.dataLength);
			//6. 处理请求并发送给客户端
			printf("Handling Client...\n");
			switch (header.cmd)
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
					header.cmd = CMD_ERROR;
					header.dataLength = 0;
					send(_cSock, (char*)&header, sizeof(DataHeader), 0);
				}
			break;
			}
		}
	}
	//7. 关闭套接字
	closesocket(_sock);
	//清除Windows socket环境
	WSACleanup();
	getchar();
	return 0;
}