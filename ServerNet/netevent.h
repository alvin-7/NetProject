#pragma once
#include <WinSock2.h>

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
	virtual void OnNetMsg(ClientSocket* pClient, const DataHeader* pHeader) = 0;
private:

};