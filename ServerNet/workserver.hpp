/*
����˺���
�ͻ������ݴ�������
������ܺʹ���ͻ�������
*/

#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include "defines.h"
#include "netevent.h"


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
		FD_ZERO(&fdMain_);//������׽��ּ������
		//FD_SET(sock_, &fdMain_);
		pThread_ = new std::thread(std::mem_fn(&CWorkServer::OnRun), this);
	}

	//����ͻ�����Ϣ
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
				printf("select -> RecvData �������\n");
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
				if (false == RecvData(fdRead_.fd_array[i])) //ʧ��������cSock
				{
					SOCKET socketTemp = fdRead_.fd_array[i];
					if (pNetEvent_)
						pNetEvent_->OnNetLeave(socketTemp);
					dClients_.erase(socketTemp);
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
		//5. ���ܿͻ�������
		int nLen = (int)recv(cSock, arrayRecv_, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			//printf("<Socket=%d>�ͻ������˳����������\n", cSock);
			return false;
		}
		ClientSocket* pClient = dClients_[cSock];
		//�����յ������ݿ�������Ϣ������ĩβ
		memcpy(pClient->getMsgBuf() + pClient->getLastPos(), arrayRecv_, nLen);
		//��Ϣ����������β��λ�ú���
		pClient->addLastPos(nLen);
		if (pClient->getLastPos() > (RECV_BUFF_SIZE * 5))
		{
			printf("���ݻ�����������������!!!\n");
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
				//printf("�յ� <Socket = %d> ���%d ���ݳ��ȣ�%d\n", cSock, header->cmd, iLen);
				if (header->cmd != 1)
				{
					system("pause");
				}
				OnNetMsg(pClient, header);
				//ʣ��δ������Ϣ���������ݳ���
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
				//��Ϣ����������һ��������Ϣ
				break;
			}
		}
		return true;
	}


	//6. �������󲢷��͸��ͻ���
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

	//��ȡfdset�пͻ�������
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

	std::map<SOCKET, ClientSocket*> dClients_;
	INetEvent* pNetEvent_;

	std::vector<SOCKET> newClients_;

public:
	std::atomic_int recvCount_;

};