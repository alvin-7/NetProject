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
#include "netevent.h"


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
		while (IsRun())
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
		int nLen = (int)recv(cSock, arrayRecv_, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			//printf("<Socket=%d>客户端已退出，任务结束\n", cSock);
			return false;
		}
		ClientSocket* pClient = dClients_[cSock];
		//将接收到的数据拷贝到消息缓冲区末尾
		memcpy(pClient->getMsgBuf() + pClient->getLastPos(), arrayRecv_, nLen);
		//消息缓冲区数据尾部位置后移
		pClient->addLastPos(nLen);
		if (pClient->getLastPos() > (RECV_BUFF_SIZE * 5))
		{
			printf("数据缓冲区溢出，程序崩溃!!!\n");
			getchar();
			return false;
		}
		//printf("nLen: %d", nLen);

		int iHandle = 0;
		int iDhLen = sizeof(DataHeader);
		while (pClient->getLastPos() >= iDhLen)
		{
			DataHeader* header = (DataHeader*)pClient->getMsgBuf();
			//printf("HEADER CMD:%d ; LEN: %d\n", header->cmd, header->dataLength);

			short iLen = header->dataLength;
			if (pClient->getLastPos() >= iLen)
			{
				//printf("收到 <Socket = %d> 命令：%d 数据长度：%d\n", cSock, header->cmd, iLen);
				if (header->cmd != 1)
				{
					system("pause");
				}
				OnNetMsg(pClient, header);
				//剩余未处理消息缓冲区数据长度
				pClient->addLastPos(-iLen);
				memcpy(pClient->getMsgBuf(), pClient->getMsgBuf() + iLen, pClient->getLastPos());

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