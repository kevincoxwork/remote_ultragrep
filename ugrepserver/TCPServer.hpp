/** @file TCPServer.hpp
	@author Kevin Cox
	@date 2019-11-26
	@version 1.0.0
	@brief TCP Server Class - Used to interact with a the "TCP Client Class"
*/
#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <future>
#include <sstream>
#include <iterator>
#include <queue>
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment (lib,"ws2_32.lib")

class TCPServer {
	//Member Variables
	bool clientConnected;
	bool serverRunning;
	unsigned short serverPort;
	std::string serverAddress;
	
	//Socket Connection Member Variables
	SOCKET serverSocketHandle;
	SOCKET acceptedClientConnectionHandle;
	
	//Networking Threads
	std::thread networkThreadRecv;
	std::thread networkThreadSend;

	//Send And Recive Storage
	std::queue<std::wstring> toSend;
	std::queue<std::wstring> resolvedResponces;
	
	//All Mutexes for The Send And Recive Storage
	std::mutex sendMutex;
	std::mutex recvMutex;
public:
	//Public Constructor and Destructor
	TCPServer(unsigned short serverPort, std::string serverAddress);
	~TCPServer();

	//Connection Lifecycle Methods
	void start();
	void stop();
	void closeClientConnection();
	
	void sendResponce(std::wstring& responce);

	std::vector<std::wstring> waitForCommand();
	std::wstring getLastResponce();
private:
	//"Threaded" inner methods
	void sendResponceThreaded();
	void receiveResponseThreaded();

	//Do not allow
	TCPServer();
};