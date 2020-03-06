#pragma once
#ifdef _WIN32
#include <WinSock2.h>
#else
#define SOCKET int
#endif // _WIN32


//客户端数据类型
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		sockfd_ = sockfd;
		memset(recvMsgBuf_, 0, sizeof(recvMsgBuf_));
		lastRecvPos_ = 0;
		lastSendPos_ = 0;
	}

	SOCKET getSockfd()
	{
		return sockfd_;
	}

	char* getMsgBuf()
	{
		return recvMsgBuf_;
	}

	int getRecvLastPos()
	{
		return lastRecvPos_;
	}
	void addRecvLastPos(int pos)
	{
		lastRecvPos_ += pos;
	}

	int getSendLastPos()
	{
		return lastSendPos_;
	}

	void addSendLastPos(int pos)
	{
		lastSendPos_ += pos;
	}

	//发送数据
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		int iLen = header->dataLength;
		const char* pSendData = (const char*)header;
		while (iLen > 0)
		{
			if (lastSendPos_ + iLen >= SEND_BUFF_SIZE)
			{
				int iLeftLen = SEND_BUFF_SIZE - lastSendPos_;
				memcpy(sendMsgBuf_, pSendData, iLeftLen);
				//剩余数据
				pSendData += iLeftLen;
				//剩余数据长度
				iLen -= iLeftLen;
				//发送整个缓冲区数据
				ret = send(sockfd_, sendMsgBuf_, SEND_BUFF_SIZE, 0);
				lastSendPos_ = 0;
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
			}
			else
			{
				memcpy(sendMsgBuf_ + lastSendPos_, pSendData, iLen);
				lastSendPos_ += iLen;
				iLen = 0;
			}
		}
		return ret;
	}

private:
	// socket fd_set  file desc set
	SOCKET sockfd_;
	//接收缓冲区
	char recvMsgBuf_[RECV_BUFF_SIZE];
	//发送缓冲区
	char sendMsgBuf_[SEND_BUFF_SIZE];
	//接收缓冲区的数据尾部位置
	int lastRecvPos_;
	//发送缓冲区的数据尾部位置
	int lastSendPos_;
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
