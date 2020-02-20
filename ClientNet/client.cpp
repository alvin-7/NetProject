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
#include <thread>
#include "defines.h"

#pragma warning(disable:4996)


class CNetClient
{
private:
	SOCKET _sock;
	fd_set fdMain;	//����һ������װsocket�Ľṹ��
public:
	CNetClient()
	{
		_sock = INVALID_SOCKET;
	}
	virtual ~CNetClient()
	{
		Close();
	}
	//��ʼ��socket
	int InitSocket()
	{
		#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		//����Windows socket����
		WSAStartup(ver, &dat);
		#endif // _WIN32
		if (INVALID_SOCKET != _sock)
		{
			printf("�رվ�����<socket = %d>��\n", _sock);
			Close();
		}
		//1. �����׽���socket
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("Socket error!\n");
			getchar();
			Close();
			return 0;
		}
		printf("Socket Success!\n");
		return 1;
	}
	//���ӵ�������
	bool Connect(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}
		//2. connect������
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
		#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		#else
		_sin.sin_addr.s_addr = inet_addr(ip);
		#endif // _WIN32

		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("Connect Error!\n");
			getchar();
			return 0;
		}
		FD_ZERO(&fdMain);//������׽��ּ������
		FD_SET(_sock, &fdMain);//���������Ȥ���׽��ֵ�����,������һ�������ݵ��׽���s
		printf("Connect Server Success!\n");
		return 1;
	}
	//�ر�socket
	void Close()
	{
		FD_ZERO(&fdMain);//������׽��ּ������

		#ifdef _WIN32
		//7. �ر��׽���
		closesocket(_sock);
		//���Windows socket����
		WSACleanup();
		#else
		close(_sock);
		#endif // _WIN32

		getchar();
	}
	//��������
	bool OnRun()
	{
		if ( !IsRun())
		{
			return false;
		}
		fd_set fdRead = fdMain;
		fd_set fdWrite = fdMain;
		fd_set fdExp = fdMain;
		timeval st = { 1, 0 };
		int ret = select(_sock + 1, &fdRead, 0, 0, &st);
		if (ret < 0)
		{
			printf("select�������\n");
			return false;
		}
		else if (0 == ret)
		{
			//printf("���д�������ҵ��\n");
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			if (0 == RecvData(_sock))
			{
				printf("select���������\n");
				return false;
			}
		}
		return true;
	}

	//�жϵ�ǰsock�Ƿ�����
	bool IsRun()
	{
		return INVALID_SOCKET != _sock && g_bRun;
	}

	//��������
	bool RecvData(SOCKET _sock)
	{
		//������
		char arrayRecv[1024] = {};
		//5. ���ܿͻ�������
		int nLen = recv(_sock, arrayRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)arrayRecv;
		if (nLen <= 0)
		{
			printf("��������Ͽ����ӣ����������\n");
			return false;
		}
		printf("�յ�<Socket = %d> ���%d ���ݳ��ȣ�%d\n", _sock, header->cmd, header->dataLength);
		recv(_sock, arrayRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(header);
		return true;
	}

	//����������Ϣ
	void OnNetMsg(DataHeader* header)
	{
		//6. ��������
		printf("Handling Client...\n");
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			LoginResult* loginRet = (LoginResult*)header;
			printf("���շ��������͵���Ϣ��%d\n", loginRet->result);
		}
		break;
		case CMD_LOGINOUT:
		{
			LoginoutResult* loginoutRet = (LoginoutResult*)header;
			printf("���շ��������͵���Ϣ��%d\n", loginoutRet->result);
		}
		break;
		default:
		{
			printf("��������������Error!\n");
		}
		break;
		}
	}

	int SendData(DataHeader * header)
	{
		if (!(IsRun() && header))
		{
			return SOCKET_ERROR;
		}
		return send(_sock, (char*)header, header->dataLength, 0);
	}
};

void CmdThread(CNetClient * client)
{
	while (g_bRun)
	{
		//3. ������������
		char cmdBuf[128] = {};
		printf("Handling...\n");
		scanf("%s", cmdBuf);
		//4. ������������
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("�յ��˳�����exit,�����˳���\n");
			getchar();
			return;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login;
			strcpy(login.uName, "name");
			strcpy(login.uPassword, "mima");
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "loginout"))
		{
			Loginout loginout;
			strcpy(loginout.uName, "name");
			client->SendData(&loginout);
		}
		else
		{
			DataHeader dh = { CMD_ERROR, 0 };
			client->SendData(&dh);
		}
	}
}

int main() 
{
	CNetClient client;
	client.InitSocket();
	bool ret = client.Connect("127.0.0.1", 7777);
	if (false == ret)
	{
		return false;
	}
	//���������߳�
	std::thread  t1(CmdThread, &client);
	t1.detach();

	while (client.IsRun())
	{
		if (!client.OnRun())
		{
			break;
		}
	}
	client.Close();
	return true;
}
