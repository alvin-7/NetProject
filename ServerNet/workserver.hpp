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
				
#ifdef _WIN32
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
#else
				if (fdRead_.fds_bits[i] == sock_)
				{
					continue;
				}
				if (false == RecvData(fdRead_.fds_bits[i])) //ʧ��������cSock
				{
					SOCKET socketTemp = fdRead_.fds_bits[i];
					if (pNetEvent_)
						pNetEvent_->OnNetLeave(socketTemp);fds_bits
					dClients_.erase(socketTemp);
					FD_CLR(socketTemp, &fdMain_);
					//�ͷ�
					close(socketTemp)
#endif // _WIN32

					
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

		int iHandle = 0;
		int iDhLen = sizeof(DataHeader);
		while (pClient->getLastPos() >= iDhLen)
		{
			DataHeader* header = (DataHeader*)pClient->getMsgBuf();

			short iLen = header->dataLength;
			if (pClient->getLastPos() >= iLen)
			{
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
	virtual void OnNetMsg(ClientSocket* pClient, const DataHeader* pHeader)
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

	//��ȡfdset�пͻ�������
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
		close(sock_);
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
public:
	std::atomic_int recvCount_;

};