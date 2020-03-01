#pragma once
#include <WinSock2.h>

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
	//virtual void OnNetMsg(SOCKET cSock) = 0;
private:

};