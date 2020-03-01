#include <stdio.h>
#include <thread>
#include <mutex>
#include <atomic>

#include "defines.h"

#pragma warning(disable:4996)

using namespace std;


class CNetClient
{
public:
	CNetClient()
	{
		sock_ = INVALID_SOCKET;
		lastPos_ = 0;
		memset(msgBuf_, 0, sizeof(msgBuf_));
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
		if (INVALID_SOCKET != sock_)
		{
			printf("�رվ�����<socket = %d>��\n", sock_);
			Close();
		}
		//1. �����׽���socket
		sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == sock_)
		{
			printf("Socket error!\n");
			//getchar();
			//Close();
			return 0;
		}
		//printf("Socket Success!\n");
		return 1;
	}
	//���ӵ�������
	bool Connect(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == sock_)
		{
			InitSocket();
		}
		//printf("\n");
		//printf("Init Done\n");
		//2. connect������
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
		#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		#else
		_sin.sin_addr.s_addr = inet_addr(ip);
		#endif // _WIN32

		int ret = connect(sock_, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("Connect Error!\n");
			//getchar();
			return 0;
		}
		//printf("Connect Done1\n");
		//printf("\n");

		isRun_ = true;
		FD_ZERO(&fdMain_);//������׽��ּ������
		FD_SET(sock_, &fdMain_);//���������Ȥ���׽��ֵ�����,������һ�������ݵ��׽���s
		//printf("Connect Server Success!\n");
		return 1;
	}
	//�ر�socket
	void Close()
	{
		isRun_ = false;
		FD_ZERO(&fdMain_);//������׽��ּ������

		#ifdef _WIN32
		//7. �ر��׽���
		closesocket(sock_);
		//���Windows socket����
		WSACleanup();
		#else
		close(sock_);
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
		fd_set fdRead = fdMain_;
		//fd_set fdWrite = fdMain_;
		//fd_set fdExp = fdMain_;
		timeval st = { 0, 10 };
		int ret = select(0, &fdRead, 0, 0, &st);
		if (ret < 0)
		{
			printf("select�������\n");
			return false;
		}
		else if (0 == ret)
		{
			printf("���д�������ҵ��\n");
			Login login;
			strcpy(login.uName, "zhuye");
			strcpy(login.uPassword, "mima");
			SendData(&login);
		}
		if (FD_ISSET(sock_, &fdRead))
		{
			FD_CLR(sock_, &fdRead);
			if (false == RecvData())
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
		return INVALID_SOCKET != sock_ && isRun_;
	}

	//��������
	bool RecvData()
	{	
		//5. ���ܿͻ�������
		int nLen = recv(sock_, arrayRecv_, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("<Socket=%d>��������Ͽ����ӣ��������\n", sock_);
			return false;
		}
		//�����յ������ݿ�������Ϣ������
		memcpy(msgBuf_ + lastPos_, arrayRecv_, nLen);
		//��Ϣ����������β��λ�ú���
		lastPos_ += nLen;
		int iHandle = 0;
		while (lastPos_ >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)msgBuf_;
			if (lastPos_ >= header->dataLength)
			{
				printf("�յ�<Socket = %d> ���%d ���ݳ��ȣ�%d\n", sock_, header->cmd, header->dataLength);
				OnNetMsg(header);
				//ʣ��δ������Ϣ���������ݳ���
				lastPos_ -= header->dataLength;
				memcpy(msgBuf_, msgBuf_ + header->dataLength, lastPos_);
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
		/*case CMD_LOGINOUT:
		{
			LoginoutResult* loginoutRet = (LoginoutResult*)header;
			printf("���շ��������͵���Ϣ��%d\n", loginoutRet->result);
		}
		break;*/
		default:
		{
			printf("��������������Error!\n");
		}
		break;
		}
	}

	int SendData(const DataHeader * header)
	{
		if (IsRun() && header)
		{
			if (header->cmd != 1 || header->dataLength != 68)
			{
				//printf("<cmd��%d   len��%d>\n", header->cmd, header->dataLength);
			}
			int iRet = SOCKET_ERROR;
			iRet = send(sock_, (const char*)header, (int)header->dataLength, 0);
			if (SOCKET_ERROR == iRet)
			{
				Close();
				return iRet;
			}
			//printf("�������ݳɹ�\n");
		}
		return 1;
	}
private:
	SOCKET sock_;
	fd_set fdMain_;	//����һ������װsocket�Ľṹ��
	//��Ϣ�����ݴ��� ��̬����
	char arrayRecv_[RECV_BUFF_SIZE] = {};
	//��Ϣ������ ��̬����
	char msgBuf_[RECV_BUFF_SIZE * 5] = {};
	//��¼�ϴν�������λ��
	int lastPos_ = 0;

	bool isRun_ = false;
	mutex mutex_;
};

//�����߳�
void CmdThread(CNetClient * client)
{
	while (true)
	{
		//3. ������������
		char cmdBuf[128] = {};
		printf("Handling...\n");
		scanf("%s", cmdBuf);
		//4. ������������
		if (0 == strcmp(cmdBuf, "exit"))
		{
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
		/*else if (0 == strcmp(cmdBuf, "loginout"))
		{
			Loginout loginout;
			strcpy(loginout.uName, "name");
			client->SendData(&loginout);
		}
		else
		{
			DataHeader dh = { CMD_ERROR, 0 };
			client->SendData(&dh);
		}*/
	}
}

bool Test();

//���Կͻ�������
const int iCount = 10000;
CNetClient* clientsLst[iCount];
mutex m;

//��ǰ4���̣߳�tidΪ1-4
bool SendThread(const int tid)
{
	printf("thread<%d>,start\n", tid);
	int iRange = iCount / _WORKCLIENT_NUM_;
	int iBegin = (tid - 1) * iRange;
	int iEnd = tid * iRange;
	//printf("iBegin = %d;\tiEND = %d\n", iBegin, iEnd);

	if (tid == _WORKCLIENT_NUM_)
	{
		iEnd += iCount % _WORKCLIENT_NUM_;
	}
	for (int i = iBegin; i < iEnd; i++)
	{
		clientsLst[i] = new CNetClient();
	}
	for (int i = iBegin; i < iEnd; i++)
	{
		clientsLst[i]->Connect("127.0.0.1", 7777);
	}
	printf("thread<%d>,connect done...\n", tid);
	
	std::chrono::milliseconds t(3000);
	std::this_thread::sleep_for(t);

	Login login;
	strcpy(login.uName, "zhuye");
	strcpy(login.uPassword, "mima");
	while (g_bRun)
	{
		//int isDisconnect = 0;
		for (int i = iBegin; i < iEnd; i++)
		{
			clientsLst[i]->SendData(&login);
			clientsLst[i]->OnRun();
		}
		//clientsLst[i]->OnRun();
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
	for (int i = iBegin; i < iEnd; i++)
	{
		clientsLst[i]->Close();
		delete clientsLst[i];
	}
	return true;
}

bool Test();

int main() 
{
	for (int i = 1; i <= _WORKCLIENT_NUM_; i++)
	{
		thread t1(SendThread, i);
		t1.detach();
	}
	while (g_bRun)
	{
		Sleep(100);
	}
	return 0;

	//return Test();

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

	Login login;
	strcpy(login.uName, "zhuye");
	strcpy(login.uPassword, "mima");
	while (g_bRun)
	{
		int isDisconnect = 0;
		for (int i = 0; i < iCount; i++)
		{
			clientsLst[i]->SendData(&login);
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