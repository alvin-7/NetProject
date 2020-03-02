#pragma once
#include <WinSock2.h>

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
	void addLastPos(int pos)
	{
		lastPos_ += pos;
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

class INetEvent
{
public:
	INetEvent()
	{}
	~INetEvent()
	{}
	//�ͻ����뿪�¼�
	virtual void OnNetLeave(SOCKET cSock) = 0;
	virtual void OnNetJoin(SOCKET cSock) = 0;
	virtual void OnNetMsg(ClientSocket* pClient, const DataHeader* pHeader) = 0;
private:

};