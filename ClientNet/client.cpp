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
	SOCKET m_sock;
	fd_set m_fdMain;	//����һ������װsocket�Ľṹ��
	//��Ϣ�����ݴ��� ��̬����
	char* m_arrayRecv = (char*)malloc(g_iRecvSize * sizeof(char));
	//��Ϣ������ ��̬����
	char* m_MsgBuf = (char*)malloc(g_iRecvSize * 2 * sizeof(char));
	//��¼�ϴν�������λ��
	int m_LastPos = 0;
public:
	CNetClient()
	{
		m_sock = INVALID_SOCKET;
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
		if (INVALID_SOCKET != m_sock)
		{
			printf("�رվ�����<socket = %d>��\n", m_sock);
			Close();
		}
		//1. �����׽���socket
		m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_sock)
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
		if (INVALID_SOCKET == m_sock)
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

		int ret = connect(m_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("Connect Error!\n");
			getchar();
			return 0;
		}
		FD_ZERO(&m_fdMain);//������׽��ּ������
		FD_SET(m_sock, &m_fdMain);//���������Ȥ���׽��ֵ�����,������һ�������ݵ��׽���s
		printf("Connect Server Success!\n");
		return 1;
	}
	//�ر�socket
	void Close()
	{
		FD_ZERO(&m_fdMain);//������׽��ּ������

		#ifdef _WIN32
		//7. �ر��׽���
		closesocket(m_sock);
		//���Windows socket����
		WSACleanup();
		#else
		close(m_sock);
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
		fd_set fdRead = m_fdMain;
		fd_set fdWrite = m_fdMain;
		fd_set fdExp = m_fdMain;
		timeval st = { 1, 0 };
		int ret = select(m_sock + 1, &fdRead, 0, 0, &st);
		if (ret < 0)
		{
			printf("select�������\n");
			return false;
		}
		else if (0 == ret)
		{
			//printf("���д�������ҵ��\n");
		}
		if (FD_ISSET(m_sock, &fdRead))
		{
			FD_CLR(m_sock, &fdRead);
			if (0 == RecvData(m_sock))
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
		return INVALID_SOCKET != m_sock && g_bRun;
	}

	//��������
	bool RecvData(SOCKET sock)
	{	
		//5. ���ܿͻ�������
		int nLen = recv(sock, m_arrayRecv, sizeof(m_arrayRecv), 0);
		if (nLen <= 0)
		{
			printf("<Socket=%d>��������Ͽ����ӣ��������\n", sock);
			return false;
		}
		//�����յ������ݿ�������Ϣ������
		memcpy(m_MsgBuf + m_LastPos, m_arrayRecv, nLen);
		//��Ϣ����������β��λ�ú���
		m_LastPos += nLen;
		int iHandle = 0;
		while (m_LastPos >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)m_MsgBuf;
			if (m_LastPos >= header->dataLength)
			{
				printf("�յ�<Socket = %d> ���%d ���ݳ��ȣ�%d\n", sock, header->cmd, header->dataLength);
				OnNetMsg(header);
				//ʣ��δ������Ϣ���������ݳ���
				m_LastPos -= header->dataLength;
				if(m_LastPos > 0)
				{
				memcpy(m_MsgBuf, m_MsgBuf + header->dataLength, m_LastPos);
				}
				iHandle += 1;
				if (0 != g_iMaxHandle and iHandle >= g_iMaxHandle)
				{
					break;
				}
			}
			else
			{
				//��Ϣ����������һ��������Ϣ
				break;
			}
		}
		return true;
	}

	//����������Ϣ
	void OnNetMsg(DataHeader* header)
	{
		//6. ��������
		printf("OnNetMsg Client...\n");
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
		return send(m_sock, (char*)header, header->dataLength, 0);
	}
};

//�����߳�
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
