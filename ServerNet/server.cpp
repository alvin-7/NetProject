#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <windows.h>
	#include <WinSock2.h>
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <string.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif // _WIN32

#include <stdio.h>
#include <vector>
#include "defines.h"

using namespace std;

class CNetServer
{
private:
	SOCKET m_sock;
	fd_set m_fdMain;		//����һ������װsocket�Ľṹ��
	fd_set fdRead = m_fdMain;

	bool m_bRun = true;		//�Ƿ�����
public:
	CNetServer()
	{
		m_sock = INVALID_SOCKET;
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
		//����Windows socket����
		WSAStartup(ver, &dat);
		#endif // _WIN32
		//1. �����׽���socket
		m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (SOCKET_ERROR == m_sock)
		{
			printf("Socket Error!\n");
			return false;
		}
		printf("Socket Success!\n");

		FD_ZERO(&m_fdMain);//������׽��ּ������
		FD_SET(m_sock, &m_fdMain);//���������Ȥ���׽��ֵ�����,������һ�������ݵ��׽���s
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
		if (SOCKET_ERROR == (bind(m_sock, (sockaddr*)&_sin, sizeof(sockaddr_in))))
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
		if (SOCKET_ERROR == listen(m_sock, num))
		{
			printf("Listen Error!\n");
			return false;
		}
		printf("Listen Success!\n");
		return true;
	}

	//���ӿͻ��ˣ�������fdRead
	bool Accept()
	{
		fdRead = m_fdMain;
		timeval st = { 1, 0 };	//select ��ʱ����

		//nfds ��һ������ֵ����ָfd_set������������������socket���ķ�Χ������������
		//���������ļ����������ֵ+1��Windowsƽ̨���Ѵ�����д0��
		//��һ����������,�Ǽ���Ŀ��,�����ǳ�ʱ��׼,select����������
		//��ȻҪ���ó�ʱ�¼�.
		//���ŵ���������Ϊfd_set�Ĳ����ֱ������ڼ���׽��ֵĿɶ���, ��д��, ��������������.
		int ret = select(m_sock + 1, &fdRead, 0, 0, &st);
		//��ֵ��select����
		if (ret < 0)
		{
			printf("selectʧ�ܣ��������\n");
			m_bRun = false;
			return false;
		}
		//�ȴ���ʱ��û�пɶ�д�������ļ�
		else if (0 == ret)
		{
			return true;
		}
		if (FD_ISSET(m_sock, &fdRead)) //�ж��ļ�������fdRead�Ƿ��ڼ���_sock��
		{
			FD_CLR(m_sock, &fdRead);
			//4. accept �ȴ����ܿͻ�������
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(clientAddr);
			SOCKET _cSock = INVALID_SOCKET;
			SOCKET iMaxSock = m_sock;
			for (u_int i = 0; i < fdRead.fd_count; i++)
			{
				if (iMaxSock < fdRead.fd_array[i])
				{
					iMaxSock = fdRead.fd_array[i];
				}
			}
			#ifdef _WIN32
			_cSock = accept(iMaxSock + 1, (sockaddr*)&clientAddr, &nAddrLen);
			#else
			_cSock = accept(iMaxSock + 1, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
			#endif // _WIN32

			if (INVALID_SOCKET == _cSock)
			{
				printf("Accept Error!\n");
			}
			else
			{
				printf("�¿ͻ��˼��� Socket: %d ; IP: %s\n", _cSock, (inet_ntoa)(clientAddr.sin_addr));
				FD_SET(_cSock, &m_fdMain);//�����׽��ֵ�����,������һ�������ݵ��׽���
			}
		}
	}

	//����ͻ�����Ϣ
	bool OnRun()
	{
		for (u_int i = 0; i < fdRead.fd_count; i++)
		{
			if (false == RecvData(fdRead.fd_array[i])) //ʧ��������cSock
			{
				SOCKET socketTemp = fdRead.fd_array[i];
				FD_CLR(socketTemp, &m_fdMain);
				//�ͷ�
				closesocket(socketTemp);
			}
		}
		return true;
	}

	//��������
	bool RecvData(SOCKET _cSock)
	{
		//������
		char arrayRecv[1024] = {};
		//5. ���ܿͻ�������
		int nLen = recv(_cSock, arrayRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)arrayRecv;
		if (nLen <= 0)
		{
			printf("�ͻ������˳������������\n");
			return false;
		}
		printf("�յ� <Socket = %d> ���%d ���ݳ��ȣ�%d\n", _cSock, header->cmd, header->dataLength);
		OnNetMsg(_cSock, header);
		return true;
	}

	//6. �������󲢷��͸��ͻ���
	virtual void OnNetMsg(SOCKET _cSock, DataHeader* header)
	{
		printf("OnNetMsg Client...\n");
		DataHeader data;
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login login = {};
			recv(_cSock, (char*)&login + sizeof(DataHeader), sizeof(Login) - sizeof(DataHeader), 0);
			printf("CMD_LOGIN name: %s ; password: %s\n", login.uName, login.uPassword);
			LoginResult ret;
			ret.result = true;
			data = (DataHeader)ret;
		}
		break;
		case CMD_LOGINOUT:
		{
			Loginout loginout = {};
			recv(_cSock, (char*)&loginout + sizeof(DataHeader), sizeof(Loginout) - sizeof(DataHeader), 0);
			printf("CMD_LOGIN name: %s\n", loginout.uName);
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
		SendData(_cSock, &data);
	}

	void SendData(SOCKET _cSock, DataHeader * header)
	{
		send(_cSock, (char*)header, header->dataLength, 0);
	}


	bool IsRun()
	{
		return m_bRun;
	}

	void Close()
	{
		#ifdef _WIN32
		//7. �ر��׽���
		closesocket(m_sock);
		//���Windows socket����
		WSACleanup();
		#else
		close(_sock);
		#endif // _WIN32
	}
};

int main()
{
	CNetServer server;
	bool bRet = server.InitSocket();
	if (false == server.Bind("127.0.0.1", 7777))
	{
		return false;
	}
	if (false == server.Listen(5))
	{
		return false;
	}
	if (!bRet)
	{
		return false;
	}
	while (server.IsRun())
	{
		server.Accept();
		server.OnRun();
		printf("���д�������ҵ��\n");
	}
	getchar();
	return true;
}