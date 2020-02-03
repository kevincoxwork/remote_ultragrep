/** @file TCPServer.cpp
	@author Kevin Cox
	@date 2019-11-26
	@version 1.0.0
	@brief TCP Server Class impl - Used to interact with a the "TCP Client Class"
*/

#include "TCPServer.hpp"

using namespace std;

//Set connection attrbutes to values to listen for client connections on
TCPServer::TCPServer(unsigned short serverPort, string serverAddress) {
	this->serverPort = serverPort;
	this->serverAddress = serverAddress;
	this->serverSocketHandle = SOCKET();
	this->acceptedClientConnectionHandle = SOCKET();
	this->clientConnected = false;
	this->serverRunning = true;
}
//Assure that we close out of each thread and handle
TCPServer::~TCPServer() {
	if (this->networkThreadSend.joinable())
		this->networkThreadSend.join();

	closeClientConnection();

	if (this->networkThreadRecv.joinable())
		this->networkThreadRecv.join();

	closesocket(this->serverSocketHandle);
	WSACleanup();

	cout << "Server Shut Down" << endl;
}
//Wait for client connection
void TCPServer::start() {
	// initialize WSA
	WSAData wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cerr << "WSAStartup failed: " << iResult << endl;
		return;
	}

	// Create the TCP socket
	this->serverSocketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Create the server address
	sockaddr_in serverAddress = { 0 };
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(this->serverPort);
	inet_pton(AF_INET, this->serverAddress.c_str(), &(serverAddress.sin_addr));

	//bind the socket
	if (::bind(this->serverSocketHandle, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		cerr << "Bind() failed." << endl;
		this->stop();
	}
	cout << "TCP/IP socket bound" << endl;
	if (listen(this->serverSocketHandle, 1) == SOCKET_ERROR) {
		cerr << "Listen() failed." << endl;
		this->stop();
	}
	//startup the recv and sending message threads
	this->networkThreadSend = thread(&TCPServer::sendResponceThreaded, this);
	this->networkThreadRecv = thread(&TCPServer::receiveResponseThreaded, this);
}
//stop the server
void TCPServer::stop() {
	wstring repsonce = L"Server Shutting Down";
	this->sendResponce(repsonce);
	this->serverRunning = false;
}
//acknowledge that the client is closing out, so close its connection here too
void TCPServer::closeClientConnection() {
	
	wstring responce = L"close-client-connection";

	this->sendResponce(responce);
	cout << "Client Connection Closed\n";
	clientConnected = false;
	closesocket(this->acceptedClientConnectionHandle);
	this->acceptedClientConnectionHandle = 0;
}
//add msg to send queue
void TCPServer::sendResponce(std::wstring& responce) {
	unique_lock<mutex> repsonceLock(sendMutex);
	this->toSend.push(responce);
}
//threaded sending thread, pick messages of queue and send to connected client
void TCPServer::sendResponceThreaded() {
	constexpr long SIZE_OF_WCHAR = sizeof(wchar_t);
	while (serverRunning) {
		while (toSend.size() > 0) {
			unique_lock<mutex> repsonceLock(sendMutex);
			wstring commandToSend = toSend.front();
			toSend.pop();


			size_t commandSize = commandToSend.size();
			send(this->acceptedClientConnectionHandle, (char*)&commandSize, sizeof(commandSize), 0);

			//wstring getServerResponce();
			int numberOfItemsSent = send(this->acceptedClientConnectionHandle, (char*)commandToSend.c_str(), SIZE_OF_WCHAR * (int)commandSize, 0);

			if (numberOfItemsSent == -1 && serverRunning) {
				//try again
				toSend.push(commandToSend);
			}
		}
	}
}
//threaded recv thread, wait for "new" client connections and exisiting client connections sending requests
void TCPServer::receiveResponseThreaded() {
	constexpr long SIZE_OF_WCHAR = sizeof(wchar_t);
	//set threading to non-blocking
	u_long nonblocking_enabled = TRUE;
	//ioctlsocket(this->acceptedClientConnectionHandle, FIONBIO, &nonblocking_enabled);
	while (serverRunning) {
		if (this->acceptedClientConnectionHandle == 0) {
			
			this->acceptedClientConnectionHandle = SOCKET_ERROR;
			std::vector<std::string> commandRecieved;
			while (this->acceptedClientConnectionHandle == SOCKET_ERROR) {
				this->acceptedClientConnectionHandle = accept(this->serverSocketHandle, NULL, NULL);
				cout << "New Connection Started\n";
				this->clientConnected = true;
			}
		}
		
		size_t sizeofString = 0;
		::recv(this->acceptedClientConnectionHandle, (char*)&sizeofString, sizeof(sizeofString), 0);
		//sometimes the string sent is a varient of a large number. Like extreamly large  -> approching size_t max. 
		// It could be reading garbage, so the max range is just some assurance
		if (sizeofString > 0 && sizeofString < 16384) {
			wstring results = L"";

			results.resize(sizeofString);
			int bytesRecieved = ::recv(this->acceptedClientConnectionHandle, (char*)results.c_str(), SIZE_OF_WCHAR * (int)sizeofString, 0);
			if (bytesRecieved != -1) {
				//try again
				
				if (results == L"closeClientConnection")
					closeClientConnection();
				else{
					unique_lock<mutex> resolvedLock(recvMutex);
					resolvedResponces.push(results);
				}
					

			}
		}
	}
}
//pick the last responce off the queue
wstring TCPServer::getLastResponce() {
	wstring responce = L"";
	if (resolvedResponces.size() > 0) {
		unique_lock<mutex> resolvedLock(recvMutex);
		responce = resolvedResponces.front();
		resolvedResponces.pop();
	}
	return responce;
}
//wait for a valid responce and then send back to the caller method
std::vector<std::wstring> TCPServer::waitForCommand() {
	//map this back into a vector object
	std::vector<std::wstring> results;

	wstring commandResult;
	while (true) {
		commandResult = getLastResponce();

		if (commandResult != L"") {
			break;
		}
	}
	//special case for grep commands as folder location could have space

	std::wstring temp;
	std::wstringstream wss(commandResult);
	int index = 0;
	while (std::getline(wss, temp, L' ')) {
		std::wstring innerResults = L"";
		

		//This had to be wrote to support folder paths with spaces in it. Folder paths must be wrapped in quotes. 
		//Beofe this, the function above would also delim the folder path and would cause an error
		if ((index == 1 || index == 2) && temp != L"-v") {
			if (temp.find(L"\"") == temp.find_last_of(L"\"")) {
				innerResults += temp;
				std::wstring innerTemp;
				while (std::getline(wss, innerTemp, L' ')) {
					
					if (innerTemp.find(L"\"") != innerTemp.find_last_of(L"\"")) {
						results.push_back(innerResults);
						break;
					}
					innerResults +=  L" " + innerTemp;
				}
				results.push_back(innerTemp);
				continue;
			}
		}
		
		results.push_back(temp);
		index++;
	}
	

	return results;
}