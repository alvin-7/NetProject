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
#include <vector>
#include "defines.h"

using namespace std;

int DoProcessor(SOCKET _cSock);

int main() {
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	//����Windows socket����
	WSAStartup(ver, &dat);
#endif // _WIN32
	//1. �����׽���socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//2. bind 
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(PORT_ZY);//host to net unsigned short
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
	_sin.sin_addr.s_addr = inet_addr("127.0.0.1");
#endif // _WIN32

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
	fd_set fdMain;	//����һ������װsocket�Ľṹ��

	FD_ZERO(&fdMain);//������׽��ּ������

	FD_SET(_sock, &fdMain);//���������Ȥ���׽��ֵ�����,������һ�������ݵ��׽���s

	while(true)
	{
		fd_set fdRead = fdMain;
		
		//nfds ��һ������ֵ����ָfd_set������������������socket���ķ�Χ������������
		//���������ļ����������ֵ+1��Windowsƽ̨���Ѵ�����д0��
		//��һ����������,�Ǽ���Ŀ��,�����ǳ�ʱ��׼,select����������
		//��ȻҪ���ó�ʱ�¼�.
		//���ŵ���������Ϊfd_set�Ĳ����ֱ������ڼ���׽��ֵĿɶ���, ��д��, ��������������.
		timeval st;
		st.tv_sec = 1;
		st.tv_usec = 0;
		int ret = select(_sock+1, &fdRead, 0, 0, &st);
		if (ret < 0)
		{
			printf("selectʧ�ܣ��������\n");
			break;
		}
		else if (0 == ret)
		{
			printf("���д�������ҵ��\n");
			continue;
		}
		if (FD_ISSET(_sock, &fdRead)) //�ж��ļ�������fdRead�Ƿ��ڼ���_sock��
		{
			FD_CLR(_sock, &fdRead);
			//4. accept �ȴ����ܿͻ�������
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(clientAddr);
			SOCKET _cSock = INVALID_SOCKET;
			SOCKET iMaxSock = _sock;
			for (u_int i = 0; i < fdRead.fd_count; i++)
			{
				if (iMaxSock < fdRead.fd_array[i])
				{
					iMaxSock = fdRead.fd_array[i];
				}
			}
#ifdef _WIN32
			_cSock = accept(iMaxSock+1, (sockaddr*)&clientAddr, &nAddrLen);
#else
			_cSock = accept(iMaxSock+1, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif // _WIN32

			if (INVALID_SOCKET == _cSock)
			{
				printf("Accept Error!\n");
			}
			else
			{
				printf("�¿ͻ��˼��� Socket: %d ; IP: %s\n", _cSock, (inet_ntoa)(clientAddr.sin_addr));
				FD_SET(_cSock, &fdMain);//�����׽��ֵ�����,������һ�������ݵ��׽���
			}
		}
		for (u_int i = 0; i < fdRead.fd_count; i++)
		{
			if (0 == DoProcessor(fdRead.fd_array[i])) //ʧ��������cSock
			{
				SOCKET socketTemp = fdRead.fd_array[i];
				FD_CLR(socketTemp, &fdMain);
				//�ͷ�
				closesocket(socketTemp);
			}
		}
		//printf("���д�������ҵ��\n");
	}
#ifdef _WIN32
	//7. �ر��׽���
	closesocket(_sock);
	//���Windows socket����
	WSACleanup();
#else
	close(_sock);
#endif // _WIN32
	getchar();
	return 0;
}

int DoProcessor(SOCKET _cSock)
{
	//������
	char arrayRecv[1024] = {};
	//5. ���ܿͻ�������
	int nLen = recv(_cSock, arrayRecv, sizeof(DataHeader), 0);
	DataHeader* header = (DataHeader*)arrayRecv;
	if (nLen <= 0)
	{
		printf("�ͻ������˳������������\n");
		return 0;
	}
	printf("�յ� <Socket = %d> ���%d ���ݳ��ȣ�%d\n", _cSock, header->cmd, header->dataLength);

	//6. �������󲢷��͸��ͻ���
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
