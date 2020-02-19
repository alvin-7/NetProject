#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <vector>
#include "defines.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;


//vector<SOCKET> g_clients;

int DoProcessor(SOCKET _cSock);

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
	_sin.sin_port = htons(PORT_ZY);//host to net unsigned short
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
	fd_set fdMain;	//创建一个用来装socket的结构体

	FD_ZERO(&fdMain);//将你的套节字集合清空

	FD_SET(_sock, &fdMain);//加入你感兴趣的套节字到集合,这里是一个读数据的套节字s

	while(true)
	{
		fd_set fdRead = fdMain;

		/*for (int i = (int)g_clients.size()-1; i >= 0; i--)
		{
			printf("FDSET\n");
			FD_SET(g_clients[i], &fdRead);
		}*/
		
		//nfds 是一个整数值，是指fd_set集合中所有描述符（socket）的范围，而不是数量
		//即是所有文件描述符最大值+1（Windows平台下已处理，可写0）
		//第一个参数不管,是兼容目的,最后的是超时标准,select是阻塞操作
		//当然要设置超时事件.
		//接着的三个类型为fd_set的参数分别是用于检查套节字的可读性, 可写性, 和列外数据性质.
		timeval st;
		st.tv_sec = 1;
		st.tv_usec = 0;
		int ret = select(0, &fdRead, 0, 0, &st);
		if (ret < 0)
		{
			printf("select失败，任务结束\n");
			break;
		}
		else if (0 == ret)
		{
			printf("空闲处理其他业务！\n");
			continue;
		}
		if (FD_ISSET(_sock, &fdRead)) //判断文件描述符fdRead是否在集合_sock中
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
			else
			{
				printf("新客户端加入 Socket: %d ; IP: %s\n", _cSock, (inet_ntoa)(clientAddr.sin_addr));
				//g_clients.push_back(_cSock);
				//FD_ZERO(&fdRead);//将你的套节字集合清空
				FD_SET(_cSock, &fdMain);//加入套节字到集合,这里是一个读数据的套节字
			}
		}
		for (u_int i = 0; i < fdRead.fd_count; i++)
		{
			if (0 == DoProcessor(fdRead.fd_array[i])) //失败则清理cSock
			{
				/*auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[i]);
				if (iter != g_clients.end())
				{
					g_clients.erase(iter);
				}*/
				SOCKET socketTemp = fdRead.fd_array[i];
				FD_CLR(socketTemp, &fdMain);
				//释放
				closesocket(socketTemp);
			}
		}
		//printf("空闲处理其他业务！\n");
	}
	/*for (u_int i = 0; i < fdMain.fd_count; i++)
	{
		SOCKET socketTemp = fdMain.fd_array[i];
		FD_CLR(socketTemp, &fdMain);
		closesocket(fdMain.fd_array[i]);
	}*/
	//7. 关闭套接字
	closesocket(_sock);
	//清除Windows socket环境
	WSACleanup();
	getchar();
	return 0;
}

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
		return 0;
	}
	printf("收到 %d 命令：%d 数据长度：%d\n", _cSock, header->cmd, header->dataLength);

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
