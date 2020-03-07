/*
服务端核心
客户端数据处理中心
负责接受和处理客户端数据
*/

#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include "defines.h"
#include "clientsock.hpp"

class CWorkServer
{
private:
	SOCKET sock_;
	//总Socket队列
	fd_set fdMain_;		//创建一个用来装socket的结构体
	//临时Socket队列
	fd_set fdRead_ = fdMain_;

	std::mutex mutex_;
	std::thread* pThread_;

	//消息接收暂存区 动态数组
	char arrayRecv_[SINGLE_BUFF_SIZE];

	bool bRun_;

	std::map<SOCKET, ClientSocket*> dClients_;
	INetEvent* pNetEvent_;
protected:
	std::atomic_int recvCount_;

public:
	CWorkServer(SOCKET sock = INVALID_SOCKET)
	{
		sock_ = sock;
		pThread_ = nullptr;
		recvCount_ = 0;
		bRun_ = true;
		pNetEvent_ = nullptr;
		memset(arrayRecv_, 0, sizeof(arrayRecv_));
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
		while (IsRun())
		{
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

#ifdef _WIN32
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
#else
				if (fdRead_.fds_bits[i] == sock_)
				{
					continue;
				}
				if (false == RecvData(fdRead_.fds_bits[i])) //失败则清理cSock
				{
					SOCKET socketTemp = fdRead_.fds_bits[i];
					if (pNetEvent_)
						pNetEvent_->OnNetLeave(socketTemp);fds_bits
						dClients_.erase(socketTemp);
					FD_CLR(socketTemp, &fdMain_);
					//释放
					close(socketTemp)
#endif // _WIN32


				}
			}
		}
		return true;
	}

	//接收数据
	bool RecvData(SOCKET cSock)
	{
		//5. 接受客户端数据
		int nLen = (int)recv(cSock, arrayRecv_, SINGLE_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			//printf("<Socket=%d>客户端已退出，任务结束\n", cSock);
			return false;
		}
		ClientSocket* pClient = dClients_[cSock];
		//将接收到的数据拷贝到消息缓冲区末尾
		memcpy(pClient->getRecvBuf() + pClient->getRecvLastPos(), arrayRecv_, nLen);
		//消息缓冲区数据尾部位置后移
		pClient->addRecvLastPos(nLen);
		if (pClient->getRecvLastPos() > RECV_BUFF_SIZE)
		{
			printf("数据缓冲区溢出，程序崩溃!!!\n");
			getchar();
			return false;
		}

		int iHandle = 0;
		int iDhLen = sizeof(DataHeader);
		while (pClient->getRecvLastPos() >= iDhLen)
		{
			DataHeader* header = (DataHeader*)pClient->getRecvBuf();

			short iLen = header->dataLength;
			if (pClient->getRecvLastPos() >= iLen)
			{
				OnNetMsg(pClient, header);
				//剩余未处理消息缓冲区数据长度
				pClient->addRecvLastPos(-iLen);
				memcpy(pClient->getRecvBuf(), pClient->getRecvBuf() + iLen, pClient->getRecvLastPos());

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
	virtual void OnNetMsg(ClientSocket * pClient, const DataHeader * pHeader)
	{
		recvCount_++;
		pNetEvent_->OnNetMsg(pClient, pHeader);
	}

	void addClient(SOCKET cSock)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		FD_SET(cSock, &fdMain_);
		dClients_[cSock] = new ClientSocket(cSock);
	}

	//获取fdset中客户端数量
	int getClientCount()
	{
		return fdMain_.fd_count;
	}

	int getRecvCount()
	{
		int iCount = recvCount_;
		recvCount_ = 0;
		return iCount;
	}

	int getSendCount()
	{
		int sendCount = 0;
		for (auto client : dClients_)
		{
			sendCount += client.second->getSendCount();
		}
		return sendCount;
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
		close(sock_);
#endif // _WIN32
	}

};
