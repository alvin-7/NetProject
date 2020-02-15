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
	if (INVALID_SOCKET == _sock)
	{
		printf("Socket error!");
		return 0;
	}
	//2. connect������
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
	while (true)
	{
		//3. ������������
		char cmdBuf[128] = {};
		scanf_s("%s", cmdBuf);
		//4. ������������
		if (0 == strcmp(cmdBuf, "exit"))
		{
			break;
		}
		else
		{
			//5. ������������
			send(_sock, cmdBuf, strlen(cmdBuf) + 1, 0);
		}
	}
	//6. ���ܷ�������Ϣ recv
	char recvBuf[256] = {};
	int nlen = recv(_sock, recvBuf, 256, 0);
	if (!nlen > 0)
	{
		printf("Recv Error!");
		return 0;
	}
	printf("���ܵ������ݣ�%s\n", recvBuf);

	//7. �ر��׽���
	closesocket(_sock);
	//���Windows socket����
	WSACleanup();
	getchar();
	return 0;
}