#include <stdio.h>
#include <thread>
#include <mutex>
#include <atomic>

#include "defines.h"

#pragma warning(disable:4996)

using namespace std;


class CNetClient
{
public:
	CNetClient()
	{
		m_Sock = INVALID_SOCKET;
		m_LastPos = 0;
		memset(m_MsgBuf, 0, sizeof(m_MsgBuf));
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
		if (INVALID_SOCKET != m_Sock)
		{
			printf("关闭旧连接<socket = %d>！\n", m_Sock);
			Close();
		}
		//1. 建立套接字socket
		m_Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_Sock)
		{
			printf("Socket error!\n");
			//getchar();
			Close();
			return 0;
		}
		//printf("Socket Success!\n");
		return 1;
	}
	//连接到服务器
	bool Connect(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == m_Sock)
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

		int ret = connect(m_Sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("Connect Error!\n");
			//getchar();
			return 0;
		}
		FD_ZERO(&m_FdMain);//将你的套节字集合清空
		FD_SET(m_Sock, &m_FdMain);//加入你感兴趣的套节字到集合,这里是一个读数据的套节字s
		//printf("Connect Server Success!\n");
		return 1;
	}
	//关闭socket
	void Close()
	{
		FD_ZERO(&m_FdMain);//将你的套节字集合清空

		#ifdef _WIN32
		//7. 关闭套接字
		closesocket(m_Sock);
		//清除Windows socket环境
		WSACleanup();
		#else
		close(m_Sock);
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
		fd_set fdRead = m_FdMain;
		//fd_set fdWrite = m_FdMain;
		//fd_set fdExp = m_FdMain;
		timeval st = { 1, 0 };
		int ret = select(0, &fdRead, 0, 0, &st);
		if (ret < 0)
		{
			printf("select程序结束\n");
			return false;
		}
		else if (0 == ret)
		{
			//printf("空闲处理其他业务！\n");
		}
		if (FD_ISSET(m_Sock, &fdRead))
		{
			FD_CLR(m_Sock, &fdRead);
			if (0 == RecvData(m_Sock))
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
		return INVALID_SOCKET != m_Sock && g_bRun;
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

	int SendData(const DataHeader * header)
	{
		if (IsRun() && header)
		{
			if (header->cmd != 1 || header->dataLength != 68)
			{
				printf("<cmd：%d   len：%d>\n", header->cmd, header->dataLength);
			}
			int iRet = send(m_Sock, (const char*)header, header->dataLength, 0);
			if (SOCKET_ERROR == iRet)
			{
				Close();
			}
		}
		return SOCKET_ERROR;
	}
private:
	SOCKET m_Sock;
	fd_set m_FdMain;	//创建一个用来装socket的结构体
	//消息接收暂存区 动态数组
	char m_ArrayRecv[RECV_BUFF_SIZE] = {};
	//消息缓冲区 动态数组
	char m_MsgBuf[RECV_BUFF_SIZE * 5] = {};
	//记录上次接收数据位置
	int m_LastPos = 0;
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

bool Test();

//测试客户端数量
const int iCount = 1000;
CNetClient* clientsLst[iCount];
mutex m;

//当前4个线程，tid为1-4
bool SendThread(const int tCount, const int tid)
{
	int iRange = iCount / tCount;
	int iBegin = (tid - 1) * iRange;
	int iEnd = tid * iRange;
	printf("iBegin = %d;\tiEND = %d\n", iBegin, iEnd);

	if (tid == tCount)
	{
		iEnd += iCount % tCount;
	}
	for (int i = iBegin; i < iEnd; i++)
	{
		if (!g_bRun)
		{
			return false;
		}
		clientsLst[i] = new CNetClient();
		clientsLst[i]->InitSocket();
	}
	for (int i = iBegin; i < iEnd; i++)
	{
		if (!g_bRun)
		{
			return false;
		}
		clientsLst[i]->Connect("127.0.0.1", 7777);
		//printf("Connect<%d> Suceess!\n", i);
	}

	Login login;
	strcpy(login.uName, "zhuye");
	strcpy(login.uPassword, "mima");
	while (g_bRun)
	{
		int isDisconnect = 0;
		for (int i = iBegin; i < iEnd; i++)
		{
			lock_guard<mutex> lg(m);
			clientsLst[i]->SendData(&login);
			//Sleep(0.001);
			/*if(!clientsLst[i]->OnRun())
			{
				isDisconnect += 1;
				if (isDisconnect >= iCount)
				{
					printf("GameOver!\n");
					getchar();
					return false;
				}
			}
			else
			{
				printf("Sending...\n");
				clientsLst[i]->SendData(&login);
			}*/
		}
	}
	for (int i = iBegin; i < iEnd; i++)
	{
		clientsLst[i]->Close();
		delete clientsLst[i];
	}
	return true;
}

bool Test();

int main() 
{
	//线程数
	//const int tCount = 4;
	//for (int i = 1; i <= tCount; i++)
	//{
	//	thread t1(SendThread, tCount, i);
	//	t1.detach();
	//}
	//while (true)
	//{
	//	//Sleep(100);
	//}
	//return 0;

	return Test();

	//CNetClient client;
	//client.InitSocket();
	//bool ret = client.Connect("127.0.0.1", 7777);
	//if (false == ret)
	//{
	//	return false;
	//}
	////启动输入线程
	//std::thread  t1(CmdThread, &client);
	//t1.detach();

	//while (client.IsRun())
	//{
	//	if (!client.OnRun())
	//	{
	//		break;
	//	}
	//}
	//client.Close();
	//return true;
}


bool Test()
{
	const int iCount = 100;
	CNetClient* clientsLst[iCount];
	for (int i = 0; i < iCount; i++)
	{
		if (!g_bRun)
		{
			return 0;
		}
		clientsLst[i] = new CNetClient();
	}
	for (int i = 0; i < iCount; i++)
	{
		if (!g_bRun)
		{
			return 0;
		}
		clientsLst[i]->Connect("127.0.0.1", 7777);
		printf("Connect<%d> Suceess!\n", i);
	}

	Login login;
	strcpy(login.uName, "zhuye");
	strcpy(login.uPassword, "mima");
	while (g_bRun)
	{
		int isDisconnect = 0;
		for (int i = 0; i < iCount; i++)
		{
			clientsLst[i]->SendData(&login);
			//Sleep(100);
			/*if(!clientsLst[i]->OnRun())
			{
				isDisconnect += 1;
				if (isDisconnect >= iCount)
				{
					printf("GameOver!\n");
					getchar();
					return false;
				}
			}
			else
			{
				printf("Sending...\n");
				clientsLst[i]->SendData(&login);
			}*/
		}
	}
	for (int i = 0; i < iCount; i++)
	{
		clientsLst[i]->Close();
		delete clientsLst[i];
	}
	return true;
}