#include "tcpserver.hpp"

class CMyServer : public CTcpServer
{
public:
	virtual void OnNetMsg(ClientSocket* pClient, const DataHeader* pHeader)
	{
		DataHeader data;
		switch (pHeader->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)pHeader;
			//printf("CMD_LOGIN name: %s ; password: %s\n", login->uName, login->uPassword);
			LoginResult ret;
			ret.result = true;
			data = (DataHeader)ret;
		}
		break;
		case CMD_LOGINOUT:
		{
			Loginout* loginout = (Loginout*)pHeader;
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
		pClient->SendData(&data);
	}

	virtual void OnNetJoin(SOCKET cSock)
	{
		CTcpServer::OnNetJoin(cSock);
	}

	virtual void OnNetLeave(SOCKET cSock)
	{
		CTcpServer::OnNetLeave(cSock);
	}
};

int main()
{
	CMyServer server;
	bool bRet = server.InitSocket();
	if (!bRet)
	{
		return false;
	}
	if (false == server.Bind("127.0.0.1", 7777))
	{
		return false;
	}
	if (false == server.Listen(5))
	{
		return false;
	}
	server.Start(4);

	while (server.IsRun())
	{
		server.OnRun();
	}
	server.Close();
	getchar();
	return true;
}

