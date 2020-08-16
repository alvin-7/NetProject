/*
tcp服务器
负责连接客户端
负责分配客户端到workserver
*/

#include "workserver.hpp"
#include "timestamp.hpp"

class CTcpServer : public INetEvent
{
private:
	//高精度计时器
	CTimestamp oTime_;

	SOCKET sock_;
	fd_set fdMain_;		//创建一个用来装socket的结构体
	fd_set fdRead_;

	std::vector<CWorkServer*> workServerLst_;


	bool bRun_;		//是否运行

protected:
	unsigned int acCount_ = 0;

public:
	CTcpServer()
	{
		sock_ = INVALID_SOCKET;
		bRun_ = true;
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
		//启动Windows socket环境
		WSAStartup(ver, &dat);
#endif // _WIN32
		if (INVALID_SOCKET != sock_)
		{
			printf("关闭旧连接<socket = %d>！\n", sock_);
			Close();
		}
		//1. 建立套接字socket
		sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (SOCKET_ERROR == int(sock_))
		{
			printf("Socket Error!\n");
			return false;
		}
		printf("Socket Success!\n");

		FD_ZERO(&fdMain_);//将你的套节字集合清空
		FD_SET(sock_, &fdMain_);//加入你感兴趣的套节字到集合,这里是一个读数据的套节字s
		return true;
	}

	//绑定ip和端口号
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

	//监听端口
	bool Listen(short num)
	{
		//3. listen,第二个参数表示默认多少人能连接
		if (SOCKET_ERROR == listen(sock_, num))
		{
			printf("Listen Error!\n");
			return false;
		}
		printf("Listen Success!\n");
		return true;
	}

	//线程开启（数量）
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

	//连接客户端，并加入fdRead
	bool Accept()
	{
		fdRead_ = fdMain_;
		timeval st = { 0, 10 };	//select 超时参数
		//nfds 是一个整数值，是指fd_set集合中所有描述符（socket）的范围，而不是数量
		//即是所有文件描述符最大值+1（Windows平台下已处理，可写0）
		//第一个参数不管,是兼容目的,最后的是超时标准,select是阻塞操作
		//当然要设置超时事件.
		//接着的三个类型为fd_set的参数分别是用于检查套节字的可读性, 可写性, 和列外数据性质.
		#ifdef _WIN32
		int ret = select(sock_ + 1, &fdRead_, nullptr, nullptr, &st);
		#else
		int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);
		#endif
		//负值：select错误
		if (ret < 0)
		{
			printf("select/epoll failure, close prog\n");
			Close();
			return false;
		}
		//等待超时，没有可读写或错误的文件
		else if (0 == ret)
		{
			return true;
		}
		if (FD_ISSET(sock_, &fdRead_)) //判断文件描述符fdRead是否在集合_sock中
		{
			FD_CLR(sock_, &fdRead_);
			//4. accept 等待接受客户端连接
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
				addClient2WorkServer(cSock);
			}
		}
		return true;
	}

	void addClient2WorkServer(SOCKET cSock)
	{
		//FD_SET(cSock, &fdMain_);//加入套节字到集合,这里是一个读数据的套节字
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

	//处理客户端消息
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

	//统计客户端数据信息
	void Time4Msg()
	{
		double t1 = oTime_.GetElapsedSecond();
		if (t1 >= 1.0)
		{
			int recvCount = 0;
			for(auto ser : workServerLst_)
			{
				recvCount += ser->getRecvCount();
			}
			int sendCount = 0;
			for (auto ser : workServerLst_)
			{
				sendCount += ser->getSendCount();
			}
			if (recvCount > 0 or sendCount > 0)
			{
				printf("clientNum: %-6d recvCount: %-10d sendCount: %-6d time: %-6lf\n", acCount_, recvCount,sendCount, t1);
			}
			else
			{
				printf("clientNum: %-6d recvCount: ZERO sendCount: ZERO  time: %lf\n", acCount_, t1);
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

		for (auto ser : workServerLst_)
		{
			delete ser;
		}
#ifdef _WIN32
		//7. 关闭套接字
		closesocket(sock_);
		//清除Windows socket环境
		WSACleanup();
#else
		close(sock_);
#endif // _WIN32
	}

	virtual void OnNetMsg(CWorkServer* pWorkServer, ClientSocket* pClient, const DataHeader* pHeader)
	{

	}

	virtual void OnNetLeave(SOCKET cSock)
	{
		acCount_--;
	}

	virtual void OnNetJoin(SOCKET cSock)
	{
		acCount_++;
	}

};
