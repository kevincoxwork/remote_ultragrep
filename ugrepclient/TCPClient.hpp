/** @file TCPClient.hpp
	@author Kevin Cox
	@date 2019-11-26
	@version 1.0.0
	@brief TCP Client Class - Used to interact with a the "TCP Server Class"
*/

#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <future>
#include <queue>

#pragma comment (lib,"ws2_32.lib")

class TCPClient {
	//Member Variables
	unsigned short remotePortNumber;
	std::string remoteAddress;
	SOCKET clientSocketHandle;
	
	//Saved Threads
	std::thread networkThreadSend;
	std::thread networkThreadRecv;
	std::thread uiThread;
	std::thread uiEnterThread;

	//Send And Recive Storage
	std::queue<std::wstring> toSend;
	std::queue<std::wstring> resolvedResponces;

	//All Mutexes for The Send And Recive Storage
	std::mutex sendMutex;
	std::mutex recvMutex;

	//UI Condition Variables
	bool clientConnectionRunning;
	bool holdTheOutput;
public:
	//Public Contructors
	TCPClient();
	TCPClient(unsigned short remotePortNumber, std::string remoteAddress);
	
	//Public Destructor
	~TCPClient();
	
	//Connect Attributes Methods
	void setRemotePortNumber(const unsigned short& remotePortNumber);
	void setRemoteAddress(std::string remoteAddress);
	void waitForResponceUI();
	
	//Connection Lifecycle Methods
	bool connectToRemote();
	void disconnectFromRemote();
	void stopRemoteServer();
	void sendRequest(std::vector<std::wstring> commandString);

	void addToResponceQueue(std::wstring string);
	
private:
	//Inner "threaded" classes
	void issueSendRequestOnThread();
	void disconnectFromRemoteResponce();
	void getServerResponceOnThread();
	void waitForEnter();
};