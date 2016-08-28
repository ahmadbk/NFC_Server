#include <winsock2.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <Psapi.h>
#include <sstream>
#include <ws2tcpip.h>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define HOST "192.168.1.72"
#define PORT "6950"
#define DEFAULT_BUFLEN 1000

//These methods are to check the if the tag exists in the database
//--------------------------------------------
string httpRequest(string, string);
bool Login(string, PCSTR, char *);
bool checkTag(string);
//--------------------------------------------

int Create_a_listening_Socket(SOCKET &ListenSocket);
int Listen_on_ListenSocket_Check_For_Client_Connect(SOCKET & ListenSocket, SOCKET & ClientSocket);

bool Receive_Data_from_Client(const SOCKET &ClientSocket, char *received_data);



int main()
{
	WSADATA wsaData;
	SOCKET ListenSocket = SOCKET_ERROR; //Socket for Server to Listen on
	SOCKET ClientSocket = SOCKET_ERROR; //Socket to store Client Connection

	bool Server_Initialised = false;
	bool Client_Connected = false;

	if (int result = WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf_s("WSAStartup failed: %d\n", result);
	}

	while (true)
	{
		//Server Initialisation
		if (!Server_Initialised) {
			Server_Initialised = Create_a_listening_Socket(ListenSocket); //Create Socket for Server to Listen On
		}
		else
		{
			//Client Connected?
			if (Client_Connected)
			{//Client Connected? --> YES
				char recvData[DEFAULT_BUFLEN];
				bool flag = Receive_Data_from_Client(ClientSocket, recvData);
				if (flag)
				{
					int i = 0;
					while (recvData[i] != NULL && i < 5)
					{
						//cout << recvData[i];
						i++;
					}
					string www(recvData);
					string tagID = www.substr(0, 3);
					cout << tagID << endl;

					if (checkTag(tagID))
					{
						cout << "Login Success\n";
					}
					else
					{
						cout << "Login not success\n";
					}
				}

				//Check if Server Reinsitialisation is Necessary
				if (!Client_Connected)
				{
					Server_Initialised = false;
				}
			}
			else
			{//Client Connected? --> NO
				cout << "Client not Connected" << endl;
				closesocket(ClientSocket);
				cout << "Wait for Client to Connect..." << endl;
				while (!Client_Connected) {
					Client_Connected = Listen_on_ListenSocket_Check_For_Client_Connect(ListenSocket, ClientSocket);
				}
			}
		}
	}

	WSACleanup();
	system("pause");

	return 0;
}

string httpRequest(string host, string tag_id)
{
	string logininfo = "tag_id=" + string(tag_id);
	int loginInfoSize = logininfo.length();
	string header;
	header = "POST /login1.php HTTP/1.1\r\n"; //Create a POST request
	header += "Host:" + host + ":80\r\n";//Works with port 80
	header += "Content-Type: application/x-www-form-urlencoded\r\n";
	header += "Content-Length: " + to_string(loginInfoSize) + "\r\n";
	header += "Accept-Charset: utf-8\r\n";
	header += "\r\n";
	header += logininfo + "\r\n";
	header += "\r\n";

	return header;
}

bool Login(string buf, PCSTR host, char *recvData) {

	struct addrinfo *ServerInfo = NULL, *prt = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	int result = getaddrinfo(host, "80", &hints, &ServerInfo);
	if (result != 0) {
		printf("getaddrinf failed: %d\n", result);
		return false;
	}
	else {
		prt = ServerInfo;
	}

	SOCKET mySocket;

	mySocket = socket(ServerInfo->ai_family, ServerInfo->ai_socktype, ServerInfo->ai_protocol);
	if (mySocket == INVALID_SOCKET) {
		printf("Error at socket() [Login]\n");
		freeaddrinfo(ServerInfo);
		return false;
	}

	int result1 = connect(mySocket, prt->ai_addr, (int)prt->ai_addrlen);
	if (result1 == SOCKET_ERROR) {
		freeaddrinfo(ServerInfo);
		closesocket(mySocket);
		cout << "Disconnected! Could Not Connect!\n" << endl;
		return false;
	}

	int result2 = send(mySocket, buf.c_str(), buf.size(), 0);
	if (result2 == SOCKET_ERROR) {
		printf("Send Failed: %d\n", WSAGetLastError());
		freeaddrinfo(ServerInfo);
		closesocket(mySocket);
		cout << "Disconnected! Could Not Send!" << endl;
		return false;
	}
	else {
		//printf("Sending...\n");
		//printf("Bytes Sent: %ld\n", result2);
	}

	result = shutdown(mySocket, SD_SEND);
	if (result == SOCKET_ERROR) {
		printf("Shutdown Failed: \n");
		freeaddrinfo(ServerInfo);
		closesocket(mySocket);
		cout << "Disconnected! Could Not shut down!" << endl;
		return false;
	}

	//recieve part
	int recvbuflen = 512;
	char recvbuf[512] = { NULL };
	result = recv(mySocket, recvbuf, recvbuflen, 0);
	if (result > 0) {
		//printf("Bytes received: %d\n", result);
		for (int i = 0; i < 512; i++)
			recvData[i] = recvbuf[i];
		//cout << "\n";
	}
	else if (result == 0)
		printf("Connection closed\n");
	else
		printf("Recieve Failed\n");

	freeaddrinfo(ServerInfo);
	closesocket(mySocket);

	return true;
}

bool checkTag(string tagID)
{
	char recv[512];
	if (Login(httpRequest(HOST, tagID), HOST, recv))
	{
		//int i = 0;
		//while (recv[i] != NULL && i < 512)
		//{
		//	cout << recv[i];
		//	i++;
		//}
		//cout << "\n";
		string res(recv);
		int num = res.find("not success");
		if(num > 0 && num < 512)
			return false;
		else
			return true;
	}
	else
		return false;
}

//Server Initialisation Subroutines
int Create_a_listening_Socket(SOCKET &ListenSocket)
{
	struct addrinfo *ServerInfo = NULL; //Pointer to Linked List of of addrinfo Structures
	struct addrinfo hints;				//For Preparation of Socket Address Structures

	int Result;  //Contain Result for System Calls

				 //Fill up  Relevant information for Address Structures
	ZeroMemory(&hints, sizeof(hints));	//Make Sure that Struct is empty
	hints.ai_family = AF_INET;			//Use IPv4
	hints.ai_socktype = SOCK_STREAM;	//Use TCP Stream Sockets
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;		//Assign Address of Local Host to Socket Structures

										//Resolve the server address and port
	Result = getaddrinfo(HOST, PORT, &hints, &ServerInfo);
	if (Result != 0) {
		printf("getaddrinfo failed with error: %d\n", Result);
		WSACleanup();
		return 0;
	}
	else
		printf("getaddrinfo OK!\n");

	//Create a SOCKET for Server to Listen On
	ListenSocket = socket(ServerInfo->ai_family, ServerInfo->ai_socktype, ServerInfo->ai_protocol);
	if (ListenSocket == SOCKET_ERROR) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(ServerInfo);
		WSACleanup();
		return 0;
	}
	else
		printf("Listen Socket creation and opening OK!\n");

	//Bind ListenSocket to Local Port
	Result = bind(ListenSocket, ServerInfo->ai_addr, (int)ServerInfo->ai_addrlen);
	if (Result == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(ServerInfo);
		closesocket(ListenSocket);
		WSACleanup();
		return 0;
	}
	else
		printf("Bind OK!\n");

	freeaddrinfo(ServerInfo); //No Longer Require Sever Address Information
	return 1;
}

int Listen_on_ListenSocket_Check_For_Client_Connect(SOCKET &ListenSocket, SOCKET &ClientSocket)
{
	int Result;

	//Set Server Listening on ListenSocket for Client Connection
	Result = listen(ListenSocket, SOMAXCONN);
	if (Result == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 0;
	}
	else {
		//ListenSocket is Listening
		ClientSocket = SOCKET_ERROR;
		ClientSocket = accept(ListenSocket, NULL, NULL);  //Look for Client Connection
		if (ClientSocket == SOCKET_ERROR) //Check if Client Connected?
		{
			closesocket(ListenSocket);
			return 0; //No client Connected
		}
		else {
			cout << "Client Connected!" << endl;
			closesocket(ListenSocket);
			return 1; //Client Connected!
		}

	}
}

bool Receive_Data_from_Client(const SOCKET &ClientSocket, char *received_data)
{
	int Result;

	Result = recv(ClientSocket, received_data, strlen(received_data), 0);
	if (Result > 0) {
		return 1; //Receive was a success
	}
	else if (Result == 0)
	{
		printf("Client Disconnected!\n"); //Client seems to have closed the connection
		return 0;
	}
	else {
		printf("recv failed with error: %d\n", WSAGetLastError());
		return 0;
	}
}