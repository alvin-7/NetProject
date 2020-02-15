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

	_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _cSock)
	{
		printf("Accept Error!\n");
	}
	printf("新客户端加入 IP: %s\n", (inet_ntoa)(clientAddr.sin_addr));

	char _recvBuf[128] = {};
	while(true)
	{
		//5. 接受客户端数据
		int nLen = recv(_cSock, _recvBuf, 128, 0);
		if (nLen <= 0)
		{
			printf("客户端已退出，任务结束！\n");
			break;
		}
		//6. 处理请求并发送给客户端
		printf("Handleing Client...\n");
		if (0 == strcmp(_recvBuf, "getName"))
		{
			char msgBuf[] = "zhuye";
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);
		}
		else if (0 == strcmp(_recvBuf, "getAge"))
		{
			char msgBuf[] = "18";
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);
		}
		else
		{
			char msgBuf[] = "Hello, Socket!";
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);
		}
	}
	//7. 关闭套接字
	closesocket(_sock);
	//清除Windows socket环境
	WSACleanup();
	getchar();
	return 0;
}