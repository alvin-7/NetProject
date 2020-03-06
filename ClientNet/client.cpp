#include <stdio.h>
#include <thread>
#include <mutex>
#include <atomic>

#include "defines.h"
#include "timestamp.hpp"

#pragma warning(disable:4996)

using namespace std;


class CNetClient
{
public:
	CNetClient()
	{
		sock_ = INVALID_SOCKET;
		lastPos_ = 0;
		recvCount_ = 0;
		memset(msgBuf_, 0, sizeof(msgBuf_));
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
		if (INVALID_SOCKET != sock_)
		{
			printf("关闭旧连接<socket = %d>！\n", sock_);
			Close();
		}
		//1. 建立套接字socket
		sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == sock_)
		{
			printf("Socket error!\n");
			return 0;
		}
		return 1;
	}
	//连接到服务器
	bool Connect(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == sock_)
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

		int ret = connect(sock_, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("Connect Error!\n");
			//getchar();
			return 0;
		}

		isRun_ = true;
		FD_ZERO(&fdMain_);//将你的套节字集合清空
		FD_SET(sock_, &fdMain_);//加入你感兴趣的套节字到集合,这里是一个读数据的套节字s
		return 1;
	}
	//关闭socket
	void Close()
	{
		isRun_ = false;
		FD_ZERO(&fdMain_);//将你的套节字集合清空

		#ifdef _WIN32
		//7. 关闭套接字
		closesocket(sock_);
		//清除Windows socket环境
		WSACleanup();
		#else
		close(sock_);
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
		fd_set fdRead = fdMain_;
		//fd_set fdWrite = fdMain_;
		//fd_set fdExp = fdMain_;
		timeval st = { 0, 10 };
		int ret = select(0, &fdRead, 0, 0, &st);
		if (ret < 0)
		{
			printf("select程序结束\n");
			return false;
		}
		else if (0 == ret)
		{
			return true;
		}
		if (FD_ISSET(sock_, &fdRead))
		{
			FD_CLR(sock_, &fdRead);
			if (false == RecvData())
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
		return INVALID_SOCKET != sock_ && isRun_;
	}

	//接受数据
	bool RecvData()
	{	
		//5. 接受客户端数据
		int nLen = recv(sock_, arrayRecv_, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("<Socket=%d>与服务器断开连接，任务结束\n", sock_);
			return false;
		}
		//将接收到的数据拷贝到消息缓冲区
		memcpy(msgBuf_ + lastPos_, arrayRecv_, nLen);
		//消息缓冲区数据尾部位置后移
		lastPos_ += nLen;
		int iHandle = 0;
		while (lastPos_ >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)msgBuf_;
			if (lastPos_ >= header->dataLength)
			{
				OnNetMsg(header);
				//剩余未处理消息缓冲区数据长度
				lastPos_ -= header->dataLength;
				memcpy(msgBuf_, msgBuf_ + header->dataLength, lastPos_);
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
		recvCount_++;
		//6. 处理请求
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			LoginResult* loginRet = (LoginResult*)header;
			//printf("接收服务器发送的信息：%d\n", loginRet->result);
		}
		break;
		/*case CMD_LOGINOUT:
		{
			LoginoutResult* loginoutRet = (LoginoutResult*)header;
			printf("接收服务器发送的信息：%d\n", loginoutRet->result);
		}
		break;*/
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
				//printf("<cmd：%d   len：%d>\n", header->cmd, header->dataLength);
			}
			int iRet = SOCKET_ERROR;
			iRet = send(sock_, (const char*)header, (int)header->dataLength, 0);
			if (SOCKET_ERROR == iRet)
			{
				Close();
				return iRet;
			}
		}
		return 1;
	}

	unsigned int getRecvCount()
	{
		auto temp = recvCount_;
		recvCount_ = 0;
		return temp;
	}
private:
	SOCKET sock_;
	fd_set fdMain_;	//创建一个用来装socket的结构体
	//消息接收暂存区 动态数组
	char arrayRecv_[RECV_BUFF_SIZE] = {};
	//消息缓冲区 动态数组
	char msgBuf_[RECV_BUFF_SIZE * 5] = {};
	//记录上次接收数据位置
	int lastPos_ = 0;

	bool isRun_ = false;
	mutex mutex_;

	unsigned int recvCount_;
};

//输入线程
void CmdThread()
{
	while (true)
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
			return;
		}
		/*else if (0 == strcmp(cmdBuf, "login"))
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
		}*/
	}
}


//测试客户端数量
const int iCount = 10000;
CNetClient* clientsLst[iCount];
mutex m;

//当前4个线程，tid为1-4
bool SendThread(const int tid)
{
	printf("thread<%d>,start\n", tid);
	int iRange = iCount / _WORKCLIENT_NUM_;
	int iBegin = (tid - 1) * iRange;
	int iEnd = tid * iRange;

	if (tid == _WORKCLIENT_NUM_)
	{
		iEnd += iCount % _WORKCLIENT_NUM_;
	}
	for (int i = iBegin; i < iEnd; i++)
	{
		clientsLst[i] = new CNetClient();
	}
	for (int i = iBegin; i < iEnd; i++)
	{
		clientsLst[i]->Connect("127.0.0.1", 7777);
	}
	printf("thread<%d>,connect done...\n", tid);
	
	/*std::chrono::milliseconds t(3000);
	std::this_thread::sleep_for(t);*/

	Login login;
	strcpy(login.uName, "zhuye");
	strcpy(login.uPassword, "mima");

	CTimestamp oTime_;
	while (g_bRun)
	{
		for (int i = iBegin; i < iEnd; i++)
		{
			clientsLst[i]->SendData(&login);
			clientsLst[i]->OnRun();
		}
		double time1 = oTime_.GetElapsedSecond();
		if (time1 > 1.0)
		{
			unsigned int allCount = 0;
			for (int i = iBegin; i < iEnd; i++)
			{
				allCount += clientsLst[i]->getRecvCount();
			}
			printf("<thread: %d>recvCount: %d, time: %lf\n", tid, allCount, time1);
			oTime_.Update();
		}
	}
	for (int i = iBegin; i < iEnd; i++)
	{
		clientsLst[i]->Close();
		delete clientsLst[i];
	}
	return true;
}


int main() 
{
	thread t(CmdThread);
	t.detach();
	for (int i = 1; i <= _WORKCLIENT_NUM_; i++)
	{
		thread t1(SendThread, i);
		t1.detach();
	}
	while (g_bRun)
	{
		Sleep(100);
	}
	getchar();
	return 0;
}