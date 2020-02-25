#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>

#include "defines.h"
#include "timestamp.hpp"


int iMaxClient = 0;

//�ͻ�����������
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		sockfd_ = sockfd;
		memset(szMsgBuf_, 0, sizeof(szMsgBuf_));
		lastPos_ = 0;
	}

	SOCKET getSockfd()
	{
		return sockfd_;
	}

	char* getMsgBuf()
	{
		return szMsgBuf_;
	}

	int getLastPos()
	{
		return lastPos_;
	}
	void setLastPos(int pos)
	{
		lastPos_ = pos;
	}

	//��������
	int SendData(DataHeader* header)
	{
		if (header)
		{
			return send(sockfd_, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

private:
	// socket fd_set  file desc set
	SOCKET sockfd_;
	//�ڶ������� ��Ϣ������
	char szMsgBuf_[RECV_BUFF_SIZE * 5];
	//��Ϣ������������β��λ��
	int lastPos_;
};

class CWorkServer
{
public:
	CWorkServer(SOCKET sock = INVALID_SOCKET)
	{
		sock_ = sock;
		pThread_ = nullptr;
		recvCount_ = 0;
		bRun_ = true;
	}
	~CWorkServer()
	{
		Close();
		sock_ = INVALID_SOCKET;
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
		int ret = select(0, &fdRead_, 0, 0, &st);
		//��ֵ��select����
		if (ret < 0)
		{
			printf("selectʧ�ܣ��������\n");
			bRun_ = false;
			return false;
		}
		//�ȴ���ʱ��û�пɶ�д�������ļ�
		else if (0 == ret)
		{
			return true;
		}
		if (FD_ISSET(sock_, &fdRead_)) //�ж��ļ�������fdRead�Ƿ��ڼ���_sock��
		{
			FD_CLR(sock_, &fdRead_);
			//4. accept �ȴ����ܿͻ�������
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(sockaddr_in);
			SOCKET cSock = INVALID_SOCKET;

#ifdef _WIN32
			cSock = accept(sock_, (sockaddr*)&clientAddr, &nAddrLen);
#else
			cSock = accept(sock_, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
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
		pThread_ = new std::thread(std::mem_fun(&CWorkServer::OnRun), this);
	}

	//����ͻ�����Ϣ
	bool OnRun()
	{
		while(IsRun())
		{
			fdRead_ = fdMain_;
			for (u_int i = 0; i < fdRead_.fd_count; i++)
			{
				if (fdRead_.fd_array[i] == sock_)
				{
					continue;
					//Accept();
				}
				if (false == RecvData(fdRead_.fd_array[i])) //ʧ��������cSock
				{
					auto iter = clients_.find(fdRead_.fd_array[i]);
					clients_.erase(iter->first);
					SOCKET socketTemp = iter->second->getSockfd();
					FD_CLR(socketTemp, &fdMain_);
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
		//memset(arrayRecv_, 0, sizeof(arrayRecv_));
		//5. ���ܿͻ�������
		int nLen = (int)recv(cSock, arrayRecv_, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("<Socket=%d>�ͻ������˳����������\n", cSock);
			return false;
		}
		//�����յ������ݿ�������Ϣ������ĩβ
		memcpy(clients_[cSock]->getMsgBuf() + clients_[cSock]->getLastPos(), arrayRecv_, nLen);
		//��Ϣ����������β��λ�ú���
		clients_[cSock]->setLastPos(clients_[cSock]->getLastPos() + nLen);
		if (clients_[cSock]->getLastPos() > (RECV_BUFF_SIZE * 5))
		{
			printf("���ݻ�����������������!!!\n");
			getchar();
			return false;
		}
		//printf("%d\n", iLastPos_);

		int iHandle = 0;
		int iDhLen = sizeof(DataHeader);
		while (clients_[cSock]->getLastPos() >= iDhLen)
		{
			DataHeader* header = (DataHeader*)clients_[cSock]->getMsgBuf();
			short iLen = header->dataLength;
			if (clients_[cSock]->getLastPos() >= iLen)
			{
				//printf("�յ� <Socket = %d> ���%d ���ݳ��ȣ�%d\n", cSock, header->cmd, iLen);
				if (header->cmd != 1)
				{
					system("pause");
				}
				OnNetMsg(cSock, header);
				//ʣ��δ������Ϣ���������ݳ���
				clients_[cSock]->setLastPos(clients_[cSock]->getLastPos() - iLen);
				memcpy(clients_[cSock]->getMsgBuf(), clients_[cSock]->getMsgBuf() + iLen, clients_[cSock]->getLastPos());

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
		recvCount_++;
		//double t1 = m_tTime.GetElapsedSecond();
		//if (t1 >= 1.0)
		//{
		//	//printf("Scoket: %d, recvCount: %d, time: %lf\n", cSock, recvCount_, t1);
		//	recvCount_ = 0;
		//	m_tTime.Update();
		//}
		//printf("OnNetMsg Client<cSock=%d>...\n", cSock);
		DataHeader data;
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//printf("CMD_LOGIN name: %s ; password: %s\n", login->uName, login->uPassword);
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
		std::lock_guard<std::mutex> lockmy(mutex_);
		FD_SET(cSock, &fdMain_);//�����׽��ֵ�����,������һ�������ݵ��׽���
		clients_[cSock] = new ClientSocket(cSock);
	}

	//��ȡfdset�пͻ�������
	int getClientCount()
	{
		return fdMain_.fd_count;
	}

	bool IsRun()
	{
		return bRun_;
	}

	void Close()
	{
#ifdef _WIN32
		//7. �ر��׽���
		closesocket(sock_);
		//���Windows socket����
		WSACleanup();
#else
		close(_sock);
#endif // _WIN32
	}

private:
	SOCKET sock_;
	//��Socket����
	fd_set fdMain_;		//����һ������װsocket�Ľṹ��
	//��ʱSocket����
	fd_set fdRead_ = fdMain_;

	std::mutex mutex_;
	std::thread* pThread_;

	//��Ϣ�����ݴ��� ��̬����
	char arrayRecv_[RECV_BUFF_SIZE];
	//��Ϣ������ ��̬����
	char msgBuf_[RECV_BUFF_SIZE * 5];

	bool bRun_;

	std::map<SOCKET, ClientSocket*> clients_;

public:
	std::atomic_int recvCount_;

};


class CNetServer
{
public:
	CNetServer()
	{
		sock_ = INVALID_SOCKET;
		iLastPos_ = 0;
		memset(msgBuf_, 0, sizeof(msgBuf_));
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
		if (INVALID_SOCKET != sock_)
		{
			printf("�رվ�����<socket = %d>��\n", sock_);
			Close();
		}
		//1. �����׽���socket
		sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (SOCKET_ERROR == sock_)
		{
			printf("Socket Error!\n");
			return false;
		}
		printf("Socket Success!\n");

		FD_ZERO(&fdMain_);//������׽��ּ������
		FD_SET(sock_, &fdMain_);//���������Ȥ���׽��ֵ�����,������һ�������ݵ��׽���s
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
		if (SOCKET_ERROR == (bind(sock_, (sockaddr*)&_sin, sizeof(sockaddr_in))))
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
		if (SOCKET_ERROR == listen(sock_, num))
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
			auto ser = new CWorkServer(sock_);
			ser->Start();
			m_WorkServerLst.push_back(ser);
		}
	}

	//���ӿͻ��ˣ�������fdRead
	bool Accept()
	{
		fdRead_ = fdMain_;
		timeval st = { 1, 0 };	//select ��ʱ����
		//nfds ��һ������ֵ����ָfd_set������������������socket���ķ�Χ������������
		//���������ļ����������ֵ+1��Windowsƽ̨���Ѵ�����д0��
		//��һ����������,�Ǽ���Ŀ��,�����ǳ�ʱ��׼,select����������
		//��ȻҪ���ó�ʱ�¼�.
		//���ŵ���������Ϊfd_set�Ĳ����ֱ������ڼ���׽��ֵĿɶ���, ��д��, ��������������.
		int ret = select(0, &fdRead_, 0, 0, &st);
		//��ֵ��select����
		if (ret < 0)
		{
			printf("selectʧ�ܣ��������\n");
			bRun_ = false;
			return false;
		}
		//�ȴ���ʱ��û�пɶ�д�������ļ�
		else if (0 == ret)
		{
			return true;
		}
		if (FD_ISSET(sock_, &fdRead_)) //�ж��ļ�������fdRead�Ƿ��ڼ���_sock��
		{
			FD_CLR(sock_, &fdRead_);
			//4. accept �ȴ����ܿͻ�������
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(sockaddr_in);
			SOCKET cSock = INVALID_SOCKET;
			
			#ifdef _WIN32
			cSock = accept(sock_, (sockaddr*)&clientAddr, &nAddrLen);
			#else
			cSock = accept(sock_, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
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
		FD_SET(cSock, &fdMain_);//�����׽��ֵ�����,������һ�������ݵ��׽���
		auto pMinServer = m_WorkServerLst[0];
		for (auto pWorkServer : m_WorkServerLst)
		{
			if (pMinServer->getClientCount() < pWorkServer->getClientCount())
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
		//for (u_int i = 0; i < fdRead_.fd_count; i++)
		//{
		//	if (false == RecvData(fdRead_.fd_array[i])) //ʧ��������cSock
		//	{
		//		SOCKET socketTemp = fdRead_.fd_array[i];
		//		FD_CLR(socketTemp, &fdMain_);
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
				recvCount += ser->recvCount_;
				ser->recvCount_ = 0;
			}
			if(recvCount > 0)
			{
				printf("recvCount: %d, time: %lf\n", recvCount, t1);
			}
			m_tTime.Update();
		}

	}

	void SendData(SOCKET cSock, DataHeader * header)
	{
		send(cSock, (char*)header, header->dataLength, 0);
	}

	bool IsRun()
	{
		return bRun_;
	}

	void Close()
	{
		#ifdef _WIN32
		//7. �ر��׽���
		closesocket(sock_);
		//���Windows socket����
		WSACleanup();
		#else
		close(_sock);
		#endif // _WIN32
	}

private:
	//�߾��ȼ�ʱ��
	CTimestamp m_tTime;
	int recvCount_ = 0;

	SOCKET sock_;
	fd_set fdMain_;		//����һ������װsocket�Ľṹ��
	fd_set fdRead_ = fdMain_;

	std::vector<CWorkServer*> m_WorkServerLst;

	//��Ϣ�����ݴ��� ��̬����
	char arrayRecv_[RECV_BUFF_SIZE] = {};
	//��Ϣ������ ��̬����
	char msgBuf_[RECV_BUFF_SIZE * 5] = {};
	//��¼�ϴν�������λ��
	int iLastPos_ = 0;

	bool bRun_ = true;		//�Ƿ�����

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

