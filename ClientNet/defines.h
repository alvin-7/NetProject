#pragma once

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

//接收消息缓冲区大小
#ifndef RECV_BUFF_SIZE
#define SINGLE_BUFF_SIZE 10240
#define RECV_BUFF_SIZE 10240*5
#endif // !RECV_BUFF_SIZE

//每帧处理消息最大数 0表示能处理无限条
#ifndef RECV_HANDLE_SIZE
#define RECV_HANDLE_SIZE 0
#endif // !RECV_HANDLE_SIZE

//客户端线程数
#define _WORKCLIENT_NUM_ 4

bool g_bRun = true;


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
	}
	bool result;
};
//
//enum CMD
//{
//	CMD_LOGIN,
//	CMD_LOGIN_RESULT,
//	CMD_LOGOUT,
//	CMD_LOGOUT_RESULT,
//	CMD_NEW_USER_JOIN,
//	CMD_ERROR
//};
//
//struct DataHeader
//{
//	DataHeader()
//	{
//		dataLength = sizeof(DataHeader);
//		cmd = CMD_ERROR;
//	}
//	short dataLength;
//	short cmd;
//};
//
////DataPackage
//struct Login : public DataHeader
//{
//	Login()
//	{
//		dataLength = sizeof(Login);
//		cmd = CMD_LOGIN;
//	}
//	char uName[32];
//	char uPassword[32];
//	char data[32];
//};
//
//struct LoginResult : public DataHeader
//{
//	LoginResult()
//	{
//		dataLength = sizeof(LoginResult);
//		cmd = CMD_LOGIN_RESULT;
//		result = 0;
//	}
//	int result;
//	char data[92];
//};
//
//struct Logout : public DataHeader
//{
//	Logout()
//	{
//		dataLength = sizeof(Logout);
//		cmd = CMD_LOGOUT;
//	}
//	char userName[32];
//};
//
//struct LogoutResult : public DataHeader
//{
//	LogoutResult()
//	{
//		dataLength = sizeof(LogoutResult);
//		cmd = CMD_LOGOUT_RESULT;
//		result = 0;
//	}
//	int result;
//};
//
//struct NewUserJoin : public DataHeader
//{
//	NewUserJoin()
//	{
//		dataLength = sizeof(NewUserJoin);
//		cmd = CMD_NEW_USER_JOIN;
//		scok = 0;
//	}
//	int scok;
//};