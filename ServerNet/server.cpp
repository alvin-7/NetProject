#include "tcpserver.hpp"

int main()
{
	CTCPServer server;
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

