#include <winsock2.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <Psapi.h>
#include <sstream>
#include <ws2tcpip.h>
#include <process.h>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define HOST "192.168.88.19"
#define PORT "6950"
#define DEFAULT_BUFLEN 1000

int numberOfClientsConnected = 0;

//These methods are to check the if the tag exists in the database
//--------------------------------------------
string httpRequest(string, string,string);
bool Connect_To_Database(string, PCSTR, char *);
bool checkTag(string,string);
//--------------------------------------------

//These methods are to setup the server to listen for incoming connectoions
//--------------------------------------------
int Create_a_listening_Socket(SOCKET &ListenSocket);
int Listen_on_ListenSocket_Check_For_Client_Connect(SOCKET & ListenSocket);
//--------------------------------------------

//These methods are to serve the client, recieve and send data to it
//--------------------------------------------
unsigned int __stdcall  ServClient(void *data);
bool Receive_Data_from_Client(const SOCKET &ClientSocket, char *received_data);
bool Send_Data_to_Client(const SOCKET &ClientSocket, const char *data_to_send, const int data_to_send_byte_length);
//--------------------------------------------

int main()
{
	cout << "Setting up Server..." << endl;
	WSADATA wsaData;
	SOCKET ListenSocket = SOCKET_ERROR; //Socket for Server to Listen on
	SOCKET ClientSocket = SOCKET_ERROR; //Socket to store Client Connection

	bool Server_Initialised = false;

	if (int result = WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf_s("WSAStartup failed: %d\n", result);
	}

	Server_Initialised = Create_a_listening_Socket(ListenSocket);

	int Result;
	Result = Listen_on_ListenSocket_Check_For_Client_Connect(ListenSocket);

	cout << "Server ready to accept new connections!" << endl << endl;

	while (ClientSocket = accept(ListenSocket, NULL, NULL))
	{
		if (ClientSocket == INVALID_SOCKET) //Check if Client Connected?
		{
			printf("invalid client socket", GetLastError());
			continue; //No client Connected
		}
		numberOfClientsConnected++;
		_beginthreadex(0, 0, ServClient, (void*)&ClientSocket, 0, 0);
	}

	WSACleanup();
	system("pause");

	return 0;
}

unsigned int __stdcall ServClient(void *data)
{
	SOCKET *client = (SOCKET *)data;
	SOCKET Client = *client;
	printf("Client %d connected\n", numberOfClientsConnected);

	char recvData[DEFAULT_BUFLEN];
	//Data should be recieved in the form of: ReaderID|TagID#
	bool flag = Receive_Data_from_Client(Client, recvData);
	if (flag)
	{
		string www(recvData);
		int delPos = www.find('|');
		int hashPos = www.find('#');
		string ReaderID = www.substr(0, delPos);
		string tagID = www.substr((delPos+1),(hashPos-delPos-1));
		cout << "Reader ID:" << ReaderID << endl;
		cout << "Tag ID:" << tagID << endl;

		//check the database to see if the tagID exists
		if (checkTag(tagID,ReaderID))
			Send_Data_to_Client(Client, "11", strlen("11"));
		else
			Send_Data_to_Client(Client, "00", strlen("00"));
		cout << "Database response sent to client " << numberOfClientsConnected << " successfully!" << endl;
	}

	closesocket(Client);
	cout << "Client " << numberOfClientsConnected << " Disconnected" << endl << endl;
	numberOfClientsConnected--;
	return 0;
}

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

int Listen_on_ListenSocket_Check_For_Client_Connect(SOCKET &ListenSocket)
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

	return 1;
}

bool Receive_Data_from_Client(const SOCKET &ClientSocket, char *received_data)
{
	int Result;

	Result = recv(ClientSocket, received_data, strlen(received_data), 0);
	if (Result > 0) {
		cout << "Data Received From Client " << numberOfClientsConnected << ":";
		int i = 0;
		while (received_data[i] != '#')
		{
			cout << received_data[i];
			i++;
		}
		cout << received_data[i] << endl;
		return 1; //Receive was a success
	}
	else if (Result == 0)
	{
		printf("Client %d Disconnected!\n", numberOfClientsConnected); //Client seems to have closed the connection
		return 0;
	}
	else {
		printf("recv failed with error: %d\n", WSAGetLastError());
		return 0;
	}
}

bool Send_Data_to_Client(const SOCKET &ClientSocket, const char *data_to_send, const int data_to_send_byte_length)
{
	//Notify Client to Wait for Data
	int iResult = send(ClientSocket, data_to_send, data_to_send_byte_length, 0);
	if (iResult == SOCKET_ERROR) {//If sending Failed
		wprintf(L"send failed with error: %d\n", WSAGetLastError());
		return 0;
	}
	else
	{	//If Sending Succeeded
		Sleep(500);
		return 1;
	}
}

string httpRequest(string host, string tag_id, string reader_id)
{
	string logininfo = "tag_id=" + string(tag_id) + "&reader_id=" + string (reader_id);
	int loginInfoSize = logininfo.length();
	string header;
	header = "POST /AddLocation.php HTTP/1.1\r\n"; //Create a POST request
	header += "Host:" + host + ":80\r\n";//Works with port 80
	header += "Content-Type: application/x-www-form-urlencoded\r\n";
	header += "Content-Length: " + to_string(loginInfoSize) + "\r\n";
	header += "Accept-Charset: utf-8\r\n";
	header += "\r\n";
	header += logininfo + "\r\n";
	header += "\r\n";

	return header;
}

bool Connect_To_Database(string buf, PCSTR host, char *recvData) {

	cout << "Checking Database...\n";

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
		cout << "Disconnected! Could Not Connect to Database\n" << endl;
		return false;
	}

	int result2 = send(mySocket, buf.c_str(), buf.size(), 0);
	if (result2 == SOCKET_ERROR) {
		printf("Send Failed: %d\n", WSAGetLastError());
		freeaddrinfo(ServerInfo);
		closesocket(mySocket);
		cout << "Disconnected! Could Not Send to Database!" << endl;
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

	cout << "Connection to database successful!\n";

	//recieve part
	int recvbuflen = 512;
	char recvbuf[512] = { NULL };
	result = recv(mySocket, recvbuf, recvbuflen, 0);
	if (result > 0) {
		for (int i = 0; i < 512; i++)
			recvData[i] = recvbuf[i];
	}
	else if (result == 0)
		printf("Connection closed\n");
	else
		printf("Recieve Failed\n");

	freeaddrinfo(ServerInfo);
	closesocket(mySocket);

	return true;
}

bool checkTag(string tagID, string readerID)
{
	char recv[512];
	if (Connect_To_Database(httpRequest(HOST, tagID,readerID), HOST, recv))
	{
		string res(recv);
		int num = res.find("not success");
		if (num > 0 && num < 512)
		{
			cout << "Database Response: not success" << endl;
			return false;
		}
		else
		{
			cout << "Database Response: success" << endl;
			return true;
		}
	}
	else
		return false;
}