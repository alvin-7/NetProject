#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

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
struct Login
{
	char uName[32];
	char uPassword[32];
};
struct LoginResult
{
	bool result;
};
struct Loginout
{
	char uName[32];
};
struct LoginoutResult
{
	bool result;
};

int main() {
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	//启动Windows socket环境
	WSAStartup(ver, &dat);
	//1. 建立套接字socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sock)
	{
		printf("Socket error!");
		getchar();
		return 0;
	}
	//2. connect服务器
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (sockaddr *)(&_sin), sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("Connect Error!");
		getchar();
		return 0;
	}
	while (true)
	{
		//3. 输入请求命令
		char cmdBuf[128] = {};
		printf("Handling...\n");
		scanf("%s", cmdBuf);
		//4. 处理请求命令
		if (0 == strcmp(cmdBuf, "exit"))
		{
			printf("收到退出命令exit,程序退出！\n");
			getchar();
			return 0;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login = { "name", "mima" };
			DataHeader dh = { CMD_LOGIN, sizeof(Login) };
			send(_sock, (char*)&dh, sizeof(DataHeader), 0);
			send(_sock, (char*)&login, sizeof(Login), 0);
		}
		else if (0 == strcmp(cmdBuf, "loginout"))
		{
			Loginout loginout = { "name" };
			DataHeader dh = { CMD_LOGINOUT, sizeof(Loginout) };
			send(_sock, (char*)&dh, sizeof(DataHeader), 0);
			send(_sock, (char*)&loginout, sizeof(Loginout), 0);
		}
		else
		{
			DataHeader dh = { CMD_ERROR, 0 };
			send(_sock, (char*)&dh, sizeof(DataHeader), 0);
		}
		//6. 接受服务器信息 recv
		DataHeader retHeader = {};
		int nlen = recv(_sock, (char*)&retHeader, sizeof(DataHeader), 0);
		if (!nlen > 0)
		{
			printf("Recv Error!\n");
		}
		else
		{
			switch (retHeader.cmd)
			{
				case CMD_LOGIN:
				{
					LoginResult loginRet = {};
					recv(_sock, (char*)&loginRet, sizeof(LoginResult), 0);
					printf("接收到的信息：%d\n", loginRet.result);
				}
				break;
			case CMD_LOGINOUT:
				{
					LoginoutResult loginoutRet = {};
					recv(_sock, (char*)&loginoutRet, sizeof(LoginoutResult), 0);
					printf("接收到的信息：%d\n", loginoutRet.result);
				}
				break;
			default:
				printf("Error!\n");
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