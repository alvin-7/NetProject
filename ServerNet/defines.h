#pragma once

#ifdef _WIN32
#define FD_SETSIZE      1024  //��Ҫ��WinSock2.h֮ǰ����
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

//������Ϣ��������С
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

//ÿ֡������Ϣ����� 0��ʾ�ܴ���������
#ifndef RECV_HANDLE_SIZE
#define RECV_HANDLE_SIZE 0
#endif // !RECV_HANDLE_SIZE

enum CMD
{
	CMD_ERROR,
	CMD_LOGIN,
	CMD_LOGINOUT,
};
struct DataHeader
{
	short cmd;
	short dataLength = sizeof(DataHeader);
};
struct Login : public DataHeader
{
	Login()
	{
		cmd = CMD_LOGIN;
		dataLength = sizeof(Login);
	}
	char uName[32];
	char uPassword[32];
};
struct LoginResult : public DataHeader
{
	LoginResult()
	{
		cmd = CMD_LOGIN;
		dataLength = sizeof(LoginResult);
		result = false;
	}
	bool result;
};
struct Loginout : public DataHeader
{
	Loginout()
	{
		cmd = CMD_LOGINOUT;
		dataLength = sizeof(Loginout);
	}
	char uName[32];
};
struct LoginoutResult : public DataHeader
{
	LoginoutResult()
	{
		cmd = CMD_LOGINOUT;
		dataLength = sizeof(LoginoutResult);
		result = false;
	}
	bool result;
};
