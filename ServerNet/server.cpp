#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>

#include "defines.h"
#include "timestamp.hpp"


class INetEvent
{
public:
	INetEvent()
	{}
	~INetEvent()
	{}
	//客户端离开事件
	virtual void OnNetLeave(SOCKET cSock) = 0;
	virtual void OnNetJoin(SOCKET cSock) = 0;
	//virtual void OnNetMsg(SOCKET cSock) = 0;
private:
	 
};


//客户端数据类型
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
	void addLastPos(int pos)
	{
		lastPos_ += pos;
	}

	//发送数据
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
	//第二缓冲区 消息缓冲区
	char szMsgBuf_[RECV_BUFF_SIZE * 5];
	//消息缓冲区的数据尾部位置
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
		pNetEvent_ = nullptr;
	}
	~CWorkServer()
	{
		Close();
		sock_ = INVALID_SOCKET;
	}

	void setEventObj(INetEvent* pEvent)
	{
		pNetEvent_ = pEvent;
	}

	void Start()
	{
		FD_ZERO(&fdMain_);//将你的套节字集合清空
		//FD_SET(sock_, &fdMain_);
		pThread_ = new std::thread(std::mem_fn(&CWorkServer::OnRun), this);
	}

	//处理客户端消息
	bool OnRun()
	{
		while(IsRun())
		{
			if (newClients_.size() > 0)
			{
				std::lock_guard<std::mutex> lock(mutex_);
				for (auto cSock : newClients_)
				{
					FD_SET(cSock, &fdMain_);
					dClients_[cSock] = new ClientSocket(cSock);
				}
				newClients_.clear();
			}
			fdRead_ = fdMain_;
			if (fdRead_.fd_count <= 0)
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			int ret = select(0, &fdRead_, nullptr, nullptr, nullptr);
			if (ret < 0)
			{
				printf("select -> RecvData 任务结束\n");
				Close();
				return false;
			}
			else if (ret == 0)
			{
				continue;
			}
			for (int i = 0; i < (int)fdRead_.fd_count; i++)
			{
				if (fdRead_.fd_array[i] == sock_)
				{
					continue;
				}
				if (false == RecvData(fdRead_.fd_array[i])) //失败则清理cSock
				{
					SOCKET socketTemp = fdRead_.fd_array[i];
					if (pNetEvent_)
						pNetEvent_->OnNetLeave(socketTemp);
					dClients_.erase(socketTemp);
					FD_CLR(socketTemp, &fdMain_);
					//释放
					closesocket(socketTemp);
				}
			}
		}
		return true;
	}

	//接收数据
	bool RecvData(SOCKET cSock)
	{
		//5. 接受客户端数据
		//std::lock_guard<std::mutex> lockmy(mutex_);

		int nLen = (int)recv(cSock, arrayRecv_, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			//printf("<Socket=%d>客户端已退出，任务结束\n", cSock);
			return false;
		}
		ClientSocket* pClien = dClients_[cSock];
		//将接收到的数据拷贝到消息缓冲区末尾
		memcpy(pClien->getMsgBuf() + pClien->getLastPos(), arrayRecv_, nLen);
		//消息缓冲区数据尾部位置后移
		pClien->addLastPos(nLen);
		if (pClien->getLastPos() > (RECV_BUFF_SIZE * 5))
		{
			printf("数据缓冲区溢出，程序崩溃!!!\n");
			getchar();
			return false;
		}
		//printf("nLen: %d", nLen);

		int iHandle = 0;
		int iDhLen = sizeof(DataHeader);
		while (pClien->getLastPos() >= iDhLen)
		{
			DataHeader* header = (DataHeader*)pClien->getMsgBuf();
			//printf("HEADER CMD:%d ; LEN: %d\n", header->cmd, header->dataLength);

			short iLen = header->dataLength;
			if (pClien->getLastPos() >= iLen)
			{
				//printf("收到 <Socket = %d> 命令：%d 数据长度：%d\n", cSock, header->cmd, iLen);
				if (header->cmd != 1)
				{
					system("pause");
				}
				OnNetMsg(pClien, header);
				//剩余未处理消息缓冲区数据长度
				pClien->addLastPos(-iLen);
				memcpy(pClien->getMsgBuf(), pClien->getMsgBuf() + iLen, pClien->getLastPos());

				iHandle += 1;
				if (0 != RECV_HANDLE_SIZE and iHandle >= RECV_HANDLE_SIZE)
				{
					printf("break\n");
					break;
				}
			}
			else
			{
				//消息缓冲区不足一条完整消息
				break;
			}
		}
		return true;
	}


	//6. 处理请求并发送给客户端
	virtual void OnNetMsg(ClientSocket* client, const DataHeader* header)
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
		client->SendData(&data);
	}

	void addClient(SOCKET cSock)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		newClients_.push_back(cSock);
	}

	//获取fdset中客户端数量
	int getClientCount()
	{
		return fdMain_.fd_count + newClients_.size();
	}

	int getRecvCount()
	{
		int iCount = recvCount_;
		recvCount_ = 0;
		return iCount;
	}

	bool IsRun()
	{
		return bRun_;
	}

	void Close()
	{
#ifdef _WIN32
		//7. 关闭套接字
		closesocket(sock_);
		//清除Windows socket环境
		WSACleanup();
#else
		close(_sock);
#endif // _WIN32
	}

private:
	SOCKET sock_;
	//总Socket队列
	fd_set fdMain_;		//创建一个用来装socket的结构体
	//临时Socket队列
	fd_set fdRead_ = fdMain_;

	std::mutex mutex_;
	std::thread* pThread_;

	//消息接收暂存区 动态数组
	char arrayRecv_[RECV_BUFF_SIZE];
	//消息缓冲区 动态数组
	char msgBuf_[RECV_BUFF_SIZE * 5];

	bool bRun_;

	std::map<SOCKET, ClientSocket*> dClients_;
	INetEvent* pNetEvent_;

	std::vector<SOCKET> newClients_;

public:
	std::atomic_int recvCount_;

};

class CNetServer : public INetEvent
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
		if (SOCKET_ERROR == sock_)
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

	void Start()
	{
		for (int i = 0; i < _WORKSERVER_NUM_; i++)
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
		int ret = select(sock_+1, &fdRead_, nullptr, nullptr, &st);
		//负值：select错误
		if (ret < 0)
		{
			printf("select -> Accept 失败，任务结束\n");
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
				//printf("<%d>新客户端加入 Socket: %d ; IP: %s\n",(int)fdMain_.fd_count, cSock, (inet_ntoa)(clientAddr.sin_addr));
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
		//for (u_int i = 0; i < fdRead_.fd_count; i++)
		//{
		//	if (false == RecvData(fdRead_.fd_array[i])) //失败则清理cSock
		//	{
		//		SOCKET socketTemp = fdRead_.fd_array[i];
		//		FD_CLR(socketTemp, &fdMain_);
		//		//释放
		//		closesocket(socketTemp);
		//	}
		//}
		return true;
	}

	//6. 处理请求并发送给客户端
	void Time4Msg()
	{
		double t1 = m_tTime.GetElapsedSecond();
		if (t1 >= 1.0)
		{
			int recvCount = 0;
			for (auto ser : workServerLst_)
			{
				int icount = ser->getRecvCount();
				recvCount += icount;
				//printf("Count : %d\n", icount );
			}
			if(recvCount > 0)
			{
				printf("clientNum: %d, recvCount: %d, time: %lf\n", acCount_, recvCount, t1);
			}
			else
			{
				printf("clientNum: %d, recvCount: ZERO, time: %lf\n", acCount_, t1);
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
		bRun_ = false;
		#ifdef _WIN32
		//7. 关闭套接字
		closesocket(sock_);
		//清除Windows socket环境
		WSACleanup();
		#else
		close(_sock);
		#endif // _WIN32
	}

	virtual void OnNetLeave(SOCKET cSock)
	{
		acCount_--;
	}
	void OnNetJoin(SOCKET cSock)
	{
		acCount_++;
	}

private:
	//高精度计时器
	CTimestamp m_tTime;
	int recvCount_ = 0;

	SOCKET sock_;
	fd_set fdMain_;		//创建一个用来装socket的结构体
	fd_set fdRead_ = fdMain_;

	std::vector<CWorkServer*> workServerLst_;

	//消息接收暂存区 动态数组
	char arrayRecv_[RECV_BUFF_SIZE] = {};
	//消息缓冲区 动态数组
	char msgBuf_[RECV_BUFF_SIZE * 5] = {};
	//记录上次接收数据位置
	int iLastPos_ = 0;

	bool bRun_ = true;		//是否运行
	unsigned int acCount_ = 0;
};


int main()
{
	CNetServer server;
	bool bRet = server.InitSocket();
	if (!bRet)
	{
		return false;
	}
	if (false == server.Bind("127.0.0.1", 7777))
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
		//printf("空闲处理其他业务！\n");
	}
	server.Close();
	getchar();
	return true;
}

