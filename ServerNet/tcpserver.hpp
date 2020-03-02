/*
tcp������
�������ӿͻ���
�������ͻ��˵�workserver
*/

#include "workserver.hpp"
#include "timestamp.hpp"

class CTcpServer : public INetEvent
{
public:
	CTcpServer()
	{
		sock_ = INVALID_SOCKET;
		iLastPos_ = 0;
		memset(msgBuf_, 0, sizeof(msgBuf_));
	}
	virtual ~CTcpServer()
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

	//�߳̿�����������
	void Start(unsigned short iThread)
	{
		for (int i = 0; i < iThread; i++)
		{
			auto ser = new CWorkServer(sock_);
			workServerLst_.push_back(ser);
			ser->setEventObj(this);
			ser->Start();
		}
	}

	//���ӿͻ��ˣ�������fdRead
	bool Accept()
	{
		fdRead_ = fdMain_;
		timeval st = { 0, 10 };	//select ��ʱ����
		//nfds ��һ������ֵ����ָfd_set������������������socket���ķ�Χ������������
		//���������ļ����������ֵ+1��Windowsƽ̨���Ѵ�����д0��
		//��һ����������,�Ǽ���Ŀ��,�����ǳ�ʱ��׼,select����������
		//��ȻҪ���ó�ʱ�¼�.
		//���ŵ���������Ϊfd_set�Ĳ����ֱ������ڼ���׽��ֵĿɶ���, ��д��, ��������������.
		int ret = select(sock_ + 1, &fdRead_, nullptr, nullptr, &st);
		//��ֵ��select����
		if (ret < 0)
		{
			printf("select -> Accept ʧ�ܣ��������\n");
			Close();
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
				//printf("<%d>�¿ͻ��˼��� Socket: %d ; IP: %s\n",(int)fdMain_.fd_count, cSock, (inet_ntoa)(clientAddr.sin_addr));
				addClient2WorkServer(cSock);
			}
		}
		return true;
	}

	void addClient2WorkServer(SOCKET cSock)
	{
		//FD_SET(cSock, &fdMain_);//�����׽��ֵ�����,������һ�������ݵ��׽���
		auto pMinServer = workServerLst_[0];
		for (auto pWorkServer : workServerLst_)
		{
			if (pWorkServer->getClientCount() < pMinServer->getClientCount())
			{
				pMinServer = pWorkServer;
			}
		}
		pMinServer->addClient(cSock);
		OnNetJoin(cSock);
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
		return true;
	}

	//ͳ�ƿͻ���������Ϣ
	void Time4Msg()
	{
		double t1 = oTime_.GetElapsedSecond();
		if (t1 >= 1.0)
		{
			int recvCount = 0;
			for (auto ser : workServerLst_)
			{
				int icount = ser->getRecvCount();
				recvCount += icount;
			}
			if (recvCount > 0)
			{
				printf("clientNum: %d, recvCount: %d, time: %lf\n", acCount_, recvCount, t1);
			}
			else
			{
				printf("clientNum: %d, recvCount: ZERO, time: %lf\n", acCount_, t1);
			}
			oTime_.Update();
		}

	}

	bool IsRun()
	{
		return bRun_;
	}

	void Close()
	{
		bRun_ = false;
#ifdef _WIN32
		//7. �ر��׽���
		closesocket(sock_);
		//���Windows socket����
		WSACleanup();
#else
		close(_sock);
#endif // _WIN32
	}

	virtual void OnNetMsg(ClientSocket* pClient, const DataHeader* pHeader)
	{

	}

	virtual void OnNetLeave(SOCKET cSock)
	{
	}

	virtual void OnNetJoin(SOCKET cSock)
	{
	}

private:
	//�߾��ȼ�ʱ��
	CTimestamp oTime_;
	int recvCount_ = 0;

	SOCKET sock_;
	fd_set fdMain_;		//����һ������װsocket�Ľṹ��
	fd_set fdRead_ = fdMain_;

	std::vector<CWorkServer*> workServerLst_;

	//��Ϣ�����ݴ��� ��̬����
	char arrayRecv_[RECV_BUFF_SIZE] = {};
	//��Ϣ������ ��̬����
	char msgBuf_[RECV_BUFF_SIZE * 5] = {};
	//��¼�ϴν�������λ��
	int iLastPos_ = 0;

	bool bRun_ = true;		//�Ƿ�����

protected:
	unsigned int acCount_ = 0;

};