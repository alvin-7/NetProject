#include <stdio.h>
#include <thread>
#include "defines.h"

#pragma warning(disable:4996)


class CNetClient
{
private:
	SOCKET m_Sock;
	fd_set m_FdMain;	//����һ������װsocket�Ľṹ��
	//��Ϣ�����ݴ��� ��̬����
	char m_ArrayRecv[RECV_BUFF_SIZE] = {};
	//��Ϣ������ ��̬����
	char m_MsgBuf[RECV_BUFF_SIZE * 2] = {};
	//��¼�ϴν�������λ��
	int m_LastPos = 0;
public:
	CNetClient()
	{
		m_Sock = INVALID_SOCKET;
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
		if (INVALID_SOCKET != m_Sock)
		{
			printf("�رվ�����<socket = %d>��\n", m_Sock);
			Close();
		}
		//1. �����׽���socket
		m_Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_Sock)
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
		if (INVALID_SOCKET == m_Sock)
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

		int ret = connect(m_Sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("Connect Error!\n");
			getchar();
			return 0;
		}
		FD_ZERO(&m_FdMain);//������׽��ּ������
		FD_SET(m_Sock, &m_FdMain);//���������Ȥ���׽��ֵ�����,������һ�������ݵ��׽���s
		printf("Connect Server Success!\n");
		return 1;
	}
	//�ر�socket
	void Close()
	{
		FD_ZERO(&m_FdMain);//������׽��ּ������

		#ifdef _WIN32
		//7. �ر��׽���
		closesocket(m_Sock);
		//���Windows socket����
		WSACleanup();
		#else
		close(m_Sock);
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
		fd_set fdRead = m_FdMain;
		fd_set fdWrite = m_FdMain;
		fd_set fdExp = m_FdMain;
		timeval st = { 1, 0 };
		int ret = select(m_Sock + 1, &fdRead, 0, 0, &st);
		if (ret < 0)
		{
			printf("select�������\n");
			return false;
		}
		else if (0 == ret)
		{
			//printf("���д�������ҵ��\n");
		}
		if (FD_ISSET(m_Sock, &fdRead))
		{
			FD_CLR(m_Sock, &fdRead);
			if (0 == RecvData(m_Sock))
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
		return INVALID_SOCKET != m_Sock && g_bRun;
	}

	//��������
	bool RecvData(SOCKET sock)
	{	
		//5. ���ܿͻ�������
		int nLen = recv(sock, m_ArrayRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("<Socket=%d>��������Ͽ����ӣ��������\n", sock);
			return false;
		}
		//�����յ������ݿ�������Ϣ������
		memcpy(m_MsgBuf + m_LastPos, m_ArrayRecv, nLen);
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
				if (0 != RECV_HANDLE_SIZE and iHandle >= RECV_HANDLE_SIZE)
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
		return send(m_Sock, (char*)header, header->dataLength, 0);
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

bool Test();

int main() 
{
	return Test();
	//CNetClient client;
	//client.InitSocket();
	//bool ret = client.Connect("127.0.0.1", 7777);
	//if (false == ret)
	//{
	//	return false;
	//}
	////���������߳�
	//std::thread  t1(CmdThread, &client);
	//t1.detach();

	//while (client.IsRun())
	//{
	//	if (!client.OnRun())
	//	{
	//		break;
	//	}
	//}
	//client.Close();
	//return true;
}

bool Test()
{
	const int iCount = 1000;
	CNetClient* clientsLst[iCount];
	for (int i = 0; i < iCount; i++)
	{
		if (!g_bRun)
		{
			return 0;
		}
		clientsLst[i] = new CNetClient();
	}
	for (int i = 0; i < iCount; i++)
	{
		if (!g_bRun)
		{
			return 0;
		}
		clientsLst[i]->Connect("127.0.0.1", 7777);
		printf("Connect<%d> Suceess!\n", i);
	}

	LoginResult loginResult;
	loginResult.result = true;
	while (g_bRun)
	{
		int isDisconnect = 0;
		for (int i = 0; i < iCount; i++)
		{
			clientsLst[i]->SendData(&loginResult);
			//Sleep(100);
			/*if(!clientsLst[i]->OnRun())
			{
				isDisconnect += 1;
				if (isDisconnect >= iCount)
				{
					printf("GameOver!\n");
					getchar();
					return false;
				}
			}
			else
			{
				printf("Sending...\n");
				clientsLst[i]->SendData(&login);
			}*/
		}
	}
	for (int i = 0; i < iCount; i++)
	{
		clientsLst[i]->Close();
		delete clientsLst[i];
	}
	return true;
}