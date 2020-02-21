#include <stdio.h>
#include <thread>
#include "defines.h"

#pragma warning(disable:4996)


class CNetClient
{
private:
	SOCKET m_sock;
	fd_set m_fdMain;	//创建一个用来装socket的结构体
	//消息接收暂存区 动态数组
	char m_ArrayRecv[RECV_BUFF_SIZE] = {};
	//消息缓冲区 动态数组
	char m_MsgBuf[RECV_BUFF_SIZE * 2] = {};
	//记录上次接收数据位置
	int m_LastPos = 0;
public:
	CNetClient()
	{
		m_sock = INVALID_SOCKET;
	}
	virtual ~CNetClient()
	{
		Close();
	}
	//初始化socket
	int InitSocket()
	{
		#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		//启动Windows socket环境
		WSAStartup(ver, &dat);
		#endif // _WIN32
		if (INVALID_SOCKET != m_sock)
		{
			printf("关闭旧连接<socket = %d>！\n", m_sock);
			Close();
		}
		//1. 建立套接字socket
		m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_sock)
		{
			printf("Socket error!\n");
			getchar();
			Close();
			return 0;
		}
		printf("Socket Success!\n");
		return 1;
	}
	//连接到服务器
	bool Connect(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == m_sock)
		{
			InitSocket();
		}
		//2. connect服务器
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
		#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		#else
		_sin.sin_addr.s_addr = inet_addr(ip);
		#endif // _WIN32

		int ret = connect(m_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("Connect Error!\n");
			getchar();
			return 0;
		}
		FD_ZERO(&m_fdMain);//将你的套节字集合清空
		FD_SET(m_sock, &m_fdMain);//加入你感兴趣的套节字到集合,这里是一个读数据的套节字s
		printf("Connect Server Success!\n");
		return 1;
	}
	//关闭socket
	void Close()
	{
		FD_ZERO(&m_fdMain);//将你的套节字集合清空

		#ifdef _WIN32
		//7. 关闭套接字
		closesocket(m_sock);
		//清除Windows socket环境
		WSACleanup();
		#else
		close(m_sock);
		#endif // _WIN32

		getchar();
	}
	//发送数据
	bool OnRun()
	{
		if ( !IsRun())
		{
			return false;
		}
		fd_set fdRead = m_fdMain;
		fd_set fdWrite = m_fdMain;
		fd_set fdExp = m_fdMain;
		timeval st = { 1, 0 };
		int ret = select(m_sock + 1, &fdRead, 0, 0, &st);
		if (ret < 0)
		{
			printf("select程序结束\n");
			return false;
		}
		else if (0 == ret)
		{
			//printf("空闲处理其他业务！\n");
		}
		if (FD_ISSET(m_sock, &fdRead))
		{
			FD_CLR(m_sock, &fdRead);
			if (0 == RecvData(m_sock))
			{
				printf("select任务结束！\n");
				return false;
			}
		}
		return true;
	}

	//判断当前sock是否正常
	bool IsRun()
	{
		return INVALID_SOCKET != m_sock && g_bRun;
	}

	//接受数据
	bool RecvData(SOCKET sock)
	{	
		//5. 接受客户端数据
		int nLen = recv(sock, m_ArrayRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("<Socket=%d>与服务器断开连接，任务结束\n", sock);
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
				printf("收到<Socket = %d> 命令：%d 数据长度：%d\n", sock, header->cmd, header->dataLength);
				OnNetMsg(header);
				//剩余未处理消息缓冲区数据长度
				m_LastPos -= header->dataLength;
				if(m_LastPos > 0)
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

	//处理网络消息
	void OnNetMsg(DataHeader* header)
	{
		//6. 处理请求
		printf("OnNetMsg Client...\n");
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			LoginResult* loginRet = (LoginResult*)header;
			printf("接收服务器发送的信息：%d\n", loginRet->result);
		}
		break;
		case CMD_LOGINOUT:
		{
			LoginoutResult* loginoutRet = (LoginoutResult*)header;
			printf("接收服务器发送的信息：%d\n", loginoutRet->result);
		}
		break;
		default:
		{
			printf("服务器发送数据Error!\n");
		}
		break;
		}
	}

	int SendData(DataHeader * header)
	{
		if (!(IsRun() && header))
		{
			return SOCKET_ERROR;
		}
		return send(m_sock, (char*)header, header->dataLength, 0);
	}
};

//输入线程
void CmdThread(CNetClient * client)
{
	while (g_bRun)
	{
		//3. 输入请求命令
		char cmdBuf[128] = {};
		printf("Handling...\n");
		scanf("%s", cmdBuf);
		//4. 处理请求命令
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("收到退出命令exit,程序退出！\n");
			getchar();
			return;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login;
			strcpy(login.uName, "name");
			strcpy(login.uPassword, "mima");
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "loginout"))
		{
			Loginout loginout;
			strcpy(loginout.uName, "name");
			client->SendData(&loginout);
		}
		else
		{
			DataHeader dh = { CMD_ERROR, 0 };
			client->SendData(&dh);
		}
	}
}

int main() 
{
	CNetClient client;
	client.InitSocket();
	bool ret = client.Connect("127.0.0.1", 7777);
	if (false == ret)
	{
		return false;
	}
	//启动输入线程
	std::thread  t1(CmdThread, &client);
	t1.detach();

	while (client.IsRun())
	{
		if (!client.OnRun())
		{
			break;
		}
	}
	client.Close();
	return true;
}
