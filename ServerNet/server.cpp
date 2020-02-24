#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "defines.h"
#include "timestamp.hpp"


int iMaxClient = 0;

class CWorkServer
{
public:
	CWorkServer(SOCKET sock = INVALID_SOCKET)
	{
		m_Sock = sock;
		m_pThread = nullptr;
		m_RecvCount = 0;
	}
	~CWorkServer()
	{
		Close();
		m_Sock = INVALID_SOCKET;
	}

	//���ӿͻ��ˣ�������fdRead
	bool Accept()
	{
		timeval st = { 1, 0 };	//select ��ʱ����
		//nfds ��һ������ֵ����ָfd_set������������������socket���ķ�Χ������������
		//���������ļ����������ֵ+1��Windowsƽ̨���Ѵ�����д0��
		//��һ����������,�Ǽ���Ŀ��,�����ǳ�ʱ��׼,select����������
		//��ȻҪ���ó�ʱ�¼�.
		//���ŵ���������Ϊfd_set�Ĳ����ֱ������ڼ���׽��ֵĿɶ���, ��д��, ��������������.
		int ret = select(0, &m_FdRead, 0, 0, &st);
		//��ֵ��select����
		if (ret < 0)
		{
			printf("selectʧ�ܣ��������\n");
			m_bRun = false;
			return false;
		}
		//�ȴ���ʱ��û�пɶ�д�������ļ�
		else if (0 == ret)
		{
			return true;
		}
		if (FD_ISSET(m_Sock, &m_FdRead)) //�ж��ļ�������fdRead�Ƿ��ڼ���_sock��
		{
			FD_CLR(m_Sock, &m_FdRead);
			//4. accept �ȴ����ܿͻ�������
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(sockaddr_in);
			SOCKET cSock = INVALID_SOCKET;

#ifdef _WIN32
			cSock = accept(m_Sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
			cSock = accept(m_Sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif // _WIN32

			if (INVALID_SOCKET == cSock)
			{
				printf("Accept Error!\n");
			}
			else
			{
				iMaxClient += 1;
				printf("<%d>�¿ͻ��˼��� Socket: %d ; IP: %s\n", iMaxClient, cSock, (inet_ntoa)(clientAddr.sin_addr));
				AddClient(cSock);
			}
		}
		return true;
	}

	void Start()
	{
		m_pThread = new std::thread(std::mem_fun(&CWorkServer::OnRun), this);
	}

	//����ͻ�����Ϣ
	bool OnRun()
	{
		while(IsRun())
		{
			m_FdRead = m_FdMain;
			for (u_int i = 0; i < m_FdRead.fd_count; i++)
			{
				if (m_FdRead.fd_array[i] == m_Sock)
				{
					Accept();
				}
				else if (false == RecvData(m_FdRead.fd_array[i])) //ʧ��������cSock
				{
					SOCKET socketTemp = m_FdRead.fd_array[i];
					FD_CLR(socketTemp, &m_FdMain);
					//�ͷ�
					closesocket(socketTemp);
				}
			}
		}
		return true;
	}

	//��������
	bool RecvData(SOCKET cSock)
	{
		//5. ���ܿͻ�������
		int nLen = recv(cSock, m_ArrayRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("<Socket=%d>�ͻ������˳����������\n", cSock);
			return false;
		}
		char oldBuf[RECV_BUFF_SIZE * 5];
		memcpy(oldBuf, m_MsgBuf, m_LastPos);
		//�����յ������ݿ�������Ϣ������ĩβ
		memcpy(m_MsgBuf + m_LastPos, m_ArrayRecv, nLen);
		//��Ϣ����������β��λ�ú���
		m_LastPos += nLen;
		if (m_LastPos > (RECV_BUFF_SIZE * 5))
		{
			printf("���ݻ�����������������!!!\n");
			getchar();
			return false;
		}
		//printf("%d\n", m_LastPos);
		int iHandle = 0;
		int iDhLen = sizeof(DataHeader);
		while (m_LastPos >= iDhLen)
		{
			DataHeader* header = (DataHeader*)m_MsgBuf;
			int iLen = header->dataLength;
			if (m_LastPos >= iLen)
			{
				printf("�յ� <Socket = %d> ���%d ���ݳ��ȣ�%d\n", cSock, header->cmd, iLen);
				if (header->cmd != 1)
				{
					printf("%s\n", oldBuf);
					printf("%s\n", m_MsgBuf);
					system("pause");
				}
				OnNetMsg(cSock, header);
				//ʣ��δ������Ϣ���������ݳ���
				m_LastPos -= iLen;
				if (m_LastPos > 0)
				{
					memcpy(m_MsgBuf, m_MsgBuf + iLen, m_LastPos);
				}
				iHandle += 1;
				if (0 != RECV_HANDLE_SIZE and iHandle >= RECV_HANDLE_SIZE)
				{
					printf("break\n");
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


	//6. �������󲢷��͸��ͻ���
	virtual void OnNetMsg(SOCKET cSock, const DataHeader* header)
	{
		m_RecvCount++;
		//double t1 = m_tTime.GetElapsedSecond();
		//if (t1 >= 1.0)
		//{
		//	//printf("Scoket: %d, recvCount: %d, time: %lf\n", cSock, m_RecvCount, t1);
		//	m_RecvCount = 0;
		//	m_tTime.Update();
		//}
		//printf("OnNetMsg Client<cSock=%d>...\n", cSock);
		DataHeader data;
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			printf("CMD_LOGIN name: %s ; password: %s\n", login->uName, login->uPassword);
			LoginResult ret;
			ret.result = true;
			data = (DataHeader)ret;
		}
		break;
		case CMD_LOGINOUT:
		{
			Loginout* loginout = (Loginout*)header;
			//printf("CMD_LOGIN name: %s\n", loginout->uName);
			LoginoutResult ret;
			ret.result = true;
			data = (DataHeader)ret;
		}
		break;
		default:
		{
			DataHeader header = {};
			header.cmd = CMD_ERROR;
			data = header;
		}
		break;
		}
		//SendData(cSock, &data);
	}

	void AddClient(SOCKET cSock)
	{
		std::lock_guard<std::mutex> lockmy(m_Mutex);
		FD_SET(cSock, &m_FdMain);//�����׽��ֵ�����,������һ�������ݵ��׽���
	}

	//��ȡfdset�пͻ�������
	int GetClientCount()
	{
		return m_FdMain.fd_count;
	}

	bool IsRun()
	{
		return m_bRun;
	}

	void Close()
	{
#ifdef _WIN32
		//7. �ر��׽���
		closesocket(m_Sock);
		//���Windows socket����
		WSACleanup();
#else
		close(_sock);
#endif // _WIN32
	}

private:
	SOCKET m_Sock;
	//��Socket����
	fd_set m_FdMain;		//����һ������װsocket�Ľṹ��
	//��ʱSocket����
	fd_set m_FdRead = m_FdMain;

	std::mutex m_Mutex;
	std::thread* m_pThread;

	//��Ϣ�����ݴ��� ��̬����
	char m_ArrayRecv[RECV_BUFF_SIZE] = {};
	//��Ϣ������ ��̬����
	char m_MsgBuf[RECV_BUFF_SIZE * 5] = {};
	//��¼�ϴν�������λ��
	int m_LastPos = 0;

	bool m_bRun = true;

public:
	std::atomic_int m_RecvCount;

};


class CNetServer
{
public:
	CNetServer()
	{
		m_Sock = INVALID_SOCKET;
		m_LastPos = 0;
		memset(m_MsgBuf, 0, sizeof(m_MsgBuf));
	}
	virtual ~CNetServer()
	{
		Close();
	}

	bool InitSocket()
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
		if (SOCKET_ERROR == m_Sock)
		{
			printf("Socket Error!\n");
			return false;
		}
		printf("Socket Success!\n");

		FD_ZERO(&m_FdMain);//������׽��ּ������
		FD_SET(m_Sock, &m_FdMain);//���������Ȥ���׽��ֵ�����,������һ�������ݵ��׽���s
		return true;
	}

	//��ip�Ͷ˿ں�
	bool Bind(const char* ip, unsigned short port)
	{
		//2. bind 
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//host to net unsigned short
		#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		#else
		_sin.sin_addr.s_addr = inet_addr(ip);
		#endif // _WIN32
		if (SOCKET_ERROR == (bind(m_Sock, (sockaddr*)&_sin, sizeof(sockaddr_in))))
		{
			printf("Bind Error!\n");
			return false;
		}
		printf("Bind Success!\n");
		return true;
	}

	//�����˿�
	bool Listen(short num)
	{
		//3. listen,�ڶ���������ʾĬ�϶�����������
		if (SOCKET_ERROR == listen(m_Sock, num))
		{
			printf("Listen Error!\n");
			return false;
		}
		printf("Listen Success!\n");
		return true;
	}

	void Start()
	{
		for (int i = 0; i < _WORKSERVER_NUM_; i++)
		{
			auto ser = new CWorkServer(m_Sock);
			ser->Start();
			m_WorkServerLst.push_back(ser);
		}
	}

	//���ӿͻ��ˣ�������fdRead
	bool Accept()
	{
		m_FdRead = m_FdMain;
		timeval st = { 1, 0 };	//select ��ʱ����
		//nfds ��һ������ֵ����ָfd_set������������������socket���ķ�Χ������������
		//���������ļ����������ֵ+1��Windowsƽ̨���Ѵ�����д0��
		//��һ����������,�Ǽ���Ŀ��,�����ǳ�ʱ��׼,select����������
		//��ȻҪ���ó�ʱ�¼�.
		//���ŵ���������Ϊfd_set�Ĳ����ֱ������ڼ���׽��ֵĿɶ���, ��д��, ��������������.
		int ret = select(0, &m_FdRead, 0, 0, &st);
		//��ֵ��select����
		if (ret < 0)
		{
			printf("selectʧ�ܣ��������\n");
			m_bRun = false;
			return false;
		}
		//�ȴ���ʱ��û�пɶ�д�������ļ�
		else if (0 == ret)
		{
			return true;
		}
		if (FD_ISSET(m_Sock, &m_FdRead)) //�ж��ļ�������fdRead�Ƿ��ڼ���_sock��
		{
			FD_CLR(m_Sock, &m_FdRead);
			//4. accept �ȴ����ܿͻ�������
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(sockaddr_in);
			SOCKET cSock = INVALID_SOCKET;
			
			#ifdef _WIN32
			cSock = accept(m_Sock, (sockaddr*)&clientAddr, &nAddrLen);
			#else
			cSock = accept(m_Sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
			#endif // _WIN32

			if (INVALID_SOCKET == cSock)
			{
				printf("Accept Error!\n");
			}
			else
			{
				iMaxClient += 1;
				printf("<%d>�¿ͻ��˼��� Socket: %d ; IP: %s\n",iMaxClient, cSock, (inet_ntoa)(clientAddr.sin_addr));
				AddClient2WorkServer(cSock);
			}
		}
		return true;
	}

	void AddClient2WorkServer(SOCKET cSock)
	{
		FD_SET(cSock, &m_FdMain);//�����׽��ֵ�����,������һ�������ݵ��׽���
		auto pMinServer = m_WorkServerLst[0];
		for (auto pWorkServer : m_WorkServerLst)
		{
			if (pMinServer->GetClientCount() < pWorkServer->GetClientCount())
			{
				pMinServer = pWorkServer;
			}
		}
		pMinServer->AddClient(cSock);
	}

	//����ͻ�����Ϣ
	bool OnRun()
	{
		Time4Msg();
		bool bRet = Accept();
		if (!bRet)
		{
			return false;
		}
		//for (u_int i = 0; i < m_FdRead.fd_count; i++)
		//{
		//	if (false == RecvData(m_FdRead.fd_array[i])) //ʧ��������cSock
		//	{
		//		SOCKET socketTemp = m_FdRead.fd_array[i];
		//		FD_CLR(socketTemp, &m_FdMain);
		//		//�ͷ�
		//		closesocket(socketTemp);
		//	}
		//}
		return true;
	}

	//6. �������󲢷��͸��ͻ���
	void Time4Msg()
	{
		double t1 = m_tTime.GetElapsedSecond();
		if (t1 >= 1.0)
		{
			int recvCount = 0;
			for (auto ser : m_WorkServerLst)
			{
				recvCount += ser->m_RecvCount;
				ser->m_RecvCount = 0;
			}
			//printf("recvCount: %d, time: %lf\n", recvCount, t1);
			m_tTime.Update();
		}

	}

	void SendData(SOCKET cSock, DataHeader * header)
	{
		send(cSock, (char*)header, header->dataLength, 0);
	}

	bool IsRun()
	{
		return m_bRun;
	}

	void Close()
	{
		#ifdef _WIN32
		//7. �ر��׽���
		closesocket(m_Sock);
		//���Windows socket����
		WSACleanup();
		#else
		close(_sock);
		#endif // _WIN32
	}

private:
	//�߾��ȼ�ʱ��
	CTimestamp m_tTime;
	int m_RecvCount = 0;

	SOCKET m_Sock;
	fd_set m_FdMain;		//����һ������װsocket�Ľṹ��
	fd_set m_FdRead = m_FdMain;

	std::vector<CWorkServer*> m_WorkServerLst;

	//��Ϣ�����ݴ��� ��̬����
	char m_ArrayRecv[RECV_BUFF_SIZE] = {};
	//��Ϣ������ ��̬����
	char m_MsgBuf[RECV_BUFF_SIZE * 5] = {};
	//��¼�ϴν�������λ��
	int m_LastPos = 0;

	bool m_bRun = true;		//�Ƿ�����

};


int main()
{
	CNetServer server;
	bool bRet = server.InitSocket();
	if (!bRet)
	{
		return false;
	}
	if (false == server.Bind("0.0.0.0", 7777))
	{
		return false;
	}
	if (false == server.Listen(5))
	{
		return false;
	}
	server.Start();

	while (server.IsRun())
	{
		server.OnRun();
		//printf("���д�������ҵ��\n");
	}
	getchar();
	return true;
}

