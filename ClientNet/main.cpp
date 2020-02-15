#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

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
		return 0;
	}
	//3. 接受服务器信息 recv
	char recvBuf[256] = {};
	int nlen = recv(_sock, recvBuf, 256, 0);
	if (!nlen > 0)
	{
		printf("Recv Error!");
		return 0;
	}
	printf("接受到的数据：%s\n", recvBuf);

	//6. 关闭套接字
	closesocket(_sock);
	//清除Windows socket环境
	WSACleanup();
	getchar();
	return 0;
}