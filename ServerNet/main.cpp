#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	//����Windows socket����
	WSAStartup(ver, &dat);
	//1. �����׽���socket
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
	//3. listen,�ڶ���������ʾĬ�϶�����������
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("Listen Error!\n");
		return 0;
	}
	//4. accept �ȴ����ܿͻ�������
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(clientAddr);
	SOCKET _cSock = INVALID_SOCKET;
	while(true)
	{ 
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
		if (INVALID_SOCKET == _cSock)
		{
			printf("Accept Error!\n");
			continue;
		}
		printf("�¿ͻ��˼��� IP: %s\n", (inet_ntoa)(clientAddr.sin_addr));
		//5. send
		char msgBuf[] = "Hello, Socket!";
		send(_cSock, msgBuf, strlen(msgBuf)+1, 0);
	}
	//6. �ر��׽���
	closesocket(_sock);
	//���Windows socket����
	WSACleanup();
	return 0;
}