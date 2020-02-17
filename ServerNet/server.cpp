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

	while (true)
	{
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
		if (INVALID_SOCKET == _cSock)
		{
			printf("Accept Error!\n");
		}
		printf("�¿ͻ��˼��� IP: %s\n", (inet_ntoa)(clientAddr.sin_addr));

		while(true)
		{
			DataHeader header = {};
			//5. ���ܿͻ�������
			int nLen = recv(_cSock, (char*)&header, sizeof(DataHeader), 0);
			if (nLen <= 0)
			{
				printf("�ͻ������˳������������\n");
				break;
			}
			printf("�յ����%d ���ݳ��ȣ�%d\n", header.cmd, header.dataLength);
			//6. �������󲢷��͸��ͻ���
			printf("Handling Client...\n");
			switch (header.cmd)
			{
				case CMD_LOGIN:
				{
					printf("ö�٣�%d", CMD_LOGIN);
					Login login = {};
					int nLen = recv(_cSock, (char*)&login, sizeof(Login), 0);
					LoginResult ret = {true};
					send(_cSock, (char*)&header, sizeof(DataHeader), 0);
					send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
				}
			break;
				case CMD_LOGINOUT:
				{
					Loginout loginout = {};
					int nLen = recv(_cSock, (char*)&loginout, sizeof(Loginout), 0);
					LoginoutResult ret = {true};
					send(_cSock, (char*)&header, sizeof(DataHeader), 0);
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
	//7. �ر��׽���
	closesocket(_sock);
	//���Windows socket����
	WSACleanup();
	getchar();
	return 0;
}