#include <stdio.h>
#include <vector>
#include "defines.h"
#include "timestamp.hpp"

using namespace std;

class CNetServer
{
private:
	//高精度计时器
	CTimestamp m_tTime;
	int m_RecvCount = 0;

	SOCKET m_Sock;
	fd_set m_FdMain;		//创建一个用来装socket的结构体
	fd_set m_FdRead = m_FdMain;

	//消息接收暂存区 动态数组
	char m_ArrayRecv[RECV_BUFF_SIZE] = {};
	//消息缓冲区 动态数组
	char m_MsgBuf[RECV_BUFF_SIZE * 2] = {};
	//记录上次接收数据位置
	int m_LastPos = 0;

	bool m_bRun = true;		//是否运行
public:
	CNetServer()
	{
		m_Sock = INVALID_SOCKET;
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
		if (INVALID_SOCKET != m_Sock)
		{
			printf("关闭旧连接<socket = %d>！\n", m_Sock);
			Close();
		}
		//1. 建立套接字socket
		m_Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (SOCKET_ERROR == m_Sock)
		{
			printf("Socket Error!\n");
			return false;
		}
		printf("Socket Success!\n");

		FD_ZERO(&m_FdMain);//将你的套节字集合清空
		FD_SET(m_Sock, &m_FdMain);//加入你感兴趣的套节字到集合,这里是一个读数据的套节字s
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
		if (SOCKET_ERROR == (bind(m_Sock, (sockaddr*)&_sin, sizeof(sockaddr_in))))
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
		if (SOCKET_ERROR == listen(m_Sock, num))
		{
			printf("Listen Error!\n");
			return false;
		}
		printf("Listen Success!\n");
		return true;
	}

	//连接客户端，并加入fdRead
	bool Accept()
	{
		m_FdRead = m_FdMain;
		timeval st = { 1, 0 };	//select 超时参数

		//nfds 是一个整数值，是指fd_set集合中所有描述符（socket）的范围，而不是数量
		//即是所有文件描述符最大值+1（Windows平台下已处理，可写0）
		//第一个参数不管,是兼容目的,最后的是超时标准,select是阻塞操作
		//当然要设置超时事件.
		//接着的三个类型为fd_set的参数分别是用于检查套节字的可读性, 可写性, 和列外数据性质.
		int ret = select(m_Sock + 1, &m_FdRead, 0, 0, &st);
		//负值：select错误
		if (ret < 0)
		{
			printf("select失败，任务结束\n");
			m_bRun = false;
			return false;
		}
		//等待超时，没有可读写或错误的文件
		else if (0 == ret)
		{
			return true;
		}
		if (FD_ISSET(m_Sock, &m_FdRead)) //判断文件描述符fdRead是否在集合_sock中
		{
			FD_CLR(m_Sock, &m_FdRead);
			//4. accept 等待接受客户端连接
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(clientAddr);
			SOCKET cSock = INVALID_SOCKET;
			SOCKET iMaxSock = m_Sock;
			for (u_int i = 0; i < m_FdRead.fd_count; i++)
			{
				if (iMaxSock < m_FdRead.fd_array[i])
				{
					iMaxSock = m_FdRead.fd_array[i];
				}
			}
#ifdef _WIN32
			cSock = accept(iMaxSock + 1, (sockaddr*)&clientAddr, &nAddrLen);
#else
			cSock = accept(iMaxSock + 1, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif // _WIN32

			if (INVALID_SOCKET == cSock)
			{
				printf("Accept Error!\n");
			}
			else
			{
				printf("新客户端加入 Socket: %d ; IP: %s\n", cSock, (inet_ntoa)(clientAddr.sin_addr));
				FD_SET(cSock, &m_FdMain);//加入套节字到集合,这里是一个读数据的套节字
			}
		}
		return true;
	}

	//处理客户端消息
	bool OnRun()
	{
		for (u_int i = 0; i < m_FdRead.fd_count; i++)
		{
			if (false == RecvData(m_FdRead.fd_array[i])) //失败则清理cSock
			{
				SOCKET socketTemp = m_FdRead.fd_array[i];
				FD_CLR(socketTemp, &m_FdMain);
				//释放
				closesocket(socketTemp);
			}
		}
		return true;
	}

	//接收数据
	bool RecvData(SOCKET cSock)
	{
		//5. 接受客户端数据
		int nLen = recv(cSock, m_ArrayRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("<Socket=%d>客户端已退出，任务结束\n", cSock);
			return false;
		}
		//将接收到的数据拷贝到消息缓冲区
		memcpy(m_MsgBuf + m_LastPos, m_ArrayRecv, nLen);
		//消息缓冲区数据尾部位置后移
		m_LastPos += nLen;
		int iHandle = 0;
		while (m_LastPos >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)m_MsgBuf;
			if (m_LastPos >= header->dataLength)
			{
				printf("收到 <Socket = %d> 命令：%d 数据长度：%d\n", cSock, header->cmd, header->dataLength);
				OnNetMsg(cSock, header);
				//剩余未处理消息缓冲区数据长度
				m_LastPos -= header->dataLength;
				if (m_LastPos > 0)
				{
					memcpy(m_MsgBuf, m_MsgBuf + header->dataLength, m_LastPos);
				}
				iHandle += 1;
				if (0 != RECV_HANDLE_SIZE and iHandle >= RECV_HANDLE_SIZE)
				{
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
	virtual void OnNetMsg(SOCKET cSock, DataHeader* header)
	{
		m_RecvCount++;
		double t1 = m_tTime.GetElapsedSecond();
		if (t1 >= 1.0)
		{
			printf("Scoket: %d, recvCount: %d, time: %lf\n", cSock, m_RecvCount, t1);
			m_tTime.Update();
		}
		printf("OnNetMsg Client<cSock=%d>...\n", cSock);
		DataHeader data;
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*) header;
			printf("CMD_LOGIN name: %s ; password: %s\n", login->uName, login->uPassword);
			LoginResult ret;
			ret.result = true;
			data = (DataHeader)ret;
		}
		break;
		case CMD_LOGINOUT:
		{
			Loginout* loginout = (Loginout*)header;
			printf("CMD_LOGIN name: %s\n", loginout->uName);
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
		SendData(cSock, &data);
	}

	void SendData(SOCKET cSock, DataHeader * header)
	{
		send(cSock, (char*)header, header->dataLength, 0);
	}


	bool IsRun()
	{
		return m_bRun;
	}

	void Close()
	{
		#ifdef _WIN32
		//7. 关闭套接字
		closesocket(m_Sock);
		//清除Windows socket环境
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
		printf("空闲处理其他业务！\n");
	}
	getchar();
	return true;
}