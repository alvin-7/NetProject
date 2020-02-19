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
	}
	bool result;
};

int DoProcessor(SOCKET _cSock);

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
		getchar();
		return 0;
	}
	//2. connect������
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(7777);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (sockaddr*)(&_sin), sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("Connect Error!");
		getchar();
		return 0;
	}
	fd_set fdMain;	//����һ������װsocket�Ľṹ��

	FD_ZERO(&fdMain);//������׽��ּ������

	FD_SET(_sock, &fdMain);//���������Ȥ���׽��ֵ�����,������һ�������ݵ��׽���s


	while (true)
	{
		fd_set fdRead = fdMain;
		timeval st;
		st.tv_sec = 1;
		st.tv_usec = 0;
		int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &st);
		if (ret < 0)
		{
			printf("select�������\n");
			break;
		}
		else if (ret == 0)
		{
			printf("���д�������ҵ��\n");
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			if (0 == DoProcessor(_sock))
			{
				printf("���������\n");
				break;
			}
		}
		////3. ������������
		//char cmdBuf[128] = {};
		//printf("Handling...\n");
		//scanf("%s", cmdBuf);
		////4. ������������
		//if (0 == strcmp(cmdBuf, "exit"))
		//{
		//	printf("�յ��˳�����exit,�����˳���\n");
		//	getchar();
		//	return 0;
		//}
		//else if (0 == strcmp(cmdBuf, "login"))
		//{
		//	Login login;
		//	strcpy(login.uName, "name");
		//	strcpy(login.uPassword, "mima");
		//	send(_sock, (char*)&login, sizeof(Login), 0);
		//}
		//else if (0 == strcmp(cmdBuf, "loginout"))
		//{
		//	Loginout loginout;
		//	strcpy(loginout.uName, "name");
		//	send(_sock, (char*)&loginout, sizeof(Loginout), 0);
		//}
		//else
		//{
		//	DataHeader dh = { CMD_ERROR, 0 };
		//	send(_sock, (char*)&dh, sizeof(DataHeader), 0);
		//}
		//6. ���ܷ�������Ϣ recv
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
				recv(_sock, (char*)&loginRet + sizeof(DataHeader), sizeof(LoginResult) - sizeof(DataHeader), 0);
				printf("���յ�����Ϣ��%d\n", loginRet.result);
			}
			break;
			case CMD_LOGINOUT:
			{
				LoginoutResult loginoutRet = {};
				recv(_sock, (char*)&loginoutRet + sizeof(DataHeader), sizeof(LoginoutResult) - sizeof(DataHeader), 0);
				printf("���յ�����Ϣ��%d\n", loginoutRet.result);
			}
			break;
			default:
				printf("Error!\n");
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

int DoProcessor(SOCKET _sock)
{
	//������
	char arrayRecv[1024] = {};
	//5. ���ܿͻ�������
	int nLen = recv(_sock, arrayRecv, sizeof(DataHeader), 0);
	DataHeader* header = (DataHeader*)arrayRecv;
	if (nLen <= 0)
	{
		printf("��������Ͽ����ӣ����������\n");
		return 0;
	}
	printf("�յ� %d ���%d ���ݳ��ȣ�%d\n", _sock, header->cmd, header->dataLength);

	//6. �������󲢷��͸��ͻ���
	printf("Handling Client...\n");
	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		recv(_sock, arrayRecv + sizeof(DataHeader), sizeof(LoginResult) - sizeof(DataHeader), 0);
		LoginResult* loginRet = (LoginResult*)arrayRecv;
		printf("���շ��������͵���Ϣ��%d\n", loginRet->result);
	}
	break;
	case CMD_LOGINOUT:
	{
		recv(_sock, arrayRecv + sizeof(DataHeader), sizeof(LoginoutResult) - sizeof(DataHeader), 0);
		LoginoutResult* loginoutRet = (LoginoutResult*)arrayRecv;
		printf("���շ��������͵���Ϣ��%d\n", loginoutRet->result);
	}
	break;
	default:
	{
		printf("��������������Error!\n");
	}
	break;
	}
	return 1;
}