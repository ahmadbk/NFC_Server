#pragma once
#include <winsock2.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <Psapi.h>
#include <sstream>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

using namespace std;

class MySock {
public:
	static MySock& Instance() {
		static MySock ms;
		return ms;
	}

	WSADATA wsaData;
	SOCKET mySocket;
	bool InitWinSock();
	struct addrinfo *results = NULL, *prt = NULL, hints;
	bool ConfigureSock();
	bool Connect();
	void Disconnect();
	bool Login(string buf);

};