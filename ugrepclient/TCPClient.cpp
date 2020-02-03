/** @file TCPClient.cpp
	@author Kevin Cox
	@date 2019-11-26
	@version 1.0.0
	@brief TCP Client Class Impl- Used to interact with a the "TCP Server Class"
*/

#include "TCPClient.hpp"

using namespace std;

//Set connection attrbutes to values but none that would actually connect to a server
TCPClient::TCPClient() {
	this->remotePortNumber = 0;
	this->remoteAddress = "0.0.0.0";
	this->clientSocketHandle = SOCKET();
	this->clientConnectionRunning = true;
	this->holdTheOutput = false;
}
//2 Arg Constructor To Specificy Port And Report Connection Address
TCPClient::TCPClient(unsigned short remotePortNumber, std::string remoteAddress) {
	this->remotePortNumber = remotePortNumber;
	this->remoteAddress = remoteAddress;
	this->clientSocketHandle = SOCKET();
	this->clientConnectionRunning = true;
	this->holdTheOutput = false;
}

//Connect to the remote instance specified in the member variable
bool TCPClient::connectToRemote() {
	// initialize WSA
	WSAData wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
		return false;

	// Create the TCP socket
	this->clientSocketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Create the server address
	sockaddr_in serverAddress = { 0 };
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(this->remotePortNumber);
	inet_pton(AF_INET, this->remoteAddress.c_str(), &(serverAddress.sin_addr));

	if (connect(this->clientSocketHandle, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		return false;
	}
	clientConnectionRunning = true;
	//Start up always running RECV and SEND Buffer
	this->networkThreadSend = thread(&TCPClient::issueSendRequestOnThread, this);
	this->networkThreadRecv = thread(&TCPClient::getServerResponceOnThread, this);

	//reset UI Thread if this client has been running before
	if (this->uiThread.joinable())
		this->uiThread.join();
	if (this->uiEnterThread.joinable())
		this->uiEnterThread.join();

	//Add the Ui Thread and "Enter Key" Thread
	//Enter Key Thread is used to detect when the enter key is "double" pressed at any time. This will allow us to enter a supplimenty command while reciving verbose input
	this->uiThread = thread(&TCPClient::waitForResponceUI, this);
	this->uiEnterThread = thread(&TCPClient::waitForEnter, this);

	return true;
}
//Add string commanf to the request queue
void TCPClient::sendRequest(std::vector<wstring> commandString) {
	wstring commandStringToSend = L"";
	for (size_t i = 0; i < commandString.size(); i++) {
		commandStringToSend += commandString[i] + L" ";
	}
	{unique_lock<mutex> guard(sendMutex);
	this->toSend.push(commandStringToSend); }
}
//Pick this request off the request queue and send it over the socket
void TCPClient::issueSendRequestOnThread() {
	constexpr long SIZE_OF_WCHAR = sizeof(wchar_t);

	while (clientConnectionRunning) {
		while (toSend.size() > 0) {
			unique_lock<mutex> guard(sendMutex);
			wstring commandString = toSend.front();

			//tell the server what to excpet
			size_t commandSize = commandString.size();
			send(this->clientSocketHandle, (char*)&commandSize, sizeof(commandSize), 0);

			int numberOfItemsSent = send(this->clientSocketHandle, (char*)commandString.c_str(), SIZE_OF_WCHAR * (int)commandSize, 0);

			if (numberOfItemsSent != -1) {
				//try again
				toSend.pop();
			}
		}
	}
}

void TCPClient::stopRemoteServer() {
	//clear the queues so this message goes out right away
	//We could also have implemented this using priority queues.  Which would have been super cool
	{unique_lock<mutex> guardSend(sendMutex);
	unique_lock<mutex> guardRecv(recvMutex);
	std::queue<wstring>().swap(toSend);
	std::queue<wstring>().swap(resolvedResponces);

	toSend.push(L"stopserver"); }
	disconnectFromRemoteResponce();
}
//Assure that we close out of all the threads and handles, perform cleanup
TCPClient::~TCPClient() {
	u_long nonblocking_enabled = TRUE;
	ioctlsocket(clientSocketHandle, FIONBIO, &nonblocking_enabled);
	int code = closesocket(this->clientSocketHandle);
	
	clientConnectionRunning = false;
	if (this->networkThreadSend.joinable())
		this->networkThreadSend.join();
	
	if (this->networkThreadRecv.joinable())
		this->networkThreadRecv.join();
	
	if (this->uiThread.joinable())
		this->uiThread.join();
	if (this->uiEnterThread.joinable())
		this->uiEnterThread.join();

	//closesocket(this->clientSocketHandle);
	WSACleanup();
}

void TCPClient::setRemotePortNumber(const unsigned short& remotePortNumber) {
	this->remotePortNumber = remotePortNumber;
}
void TCPClient::setRemoteAddress(std::string remoteAddress) {
	this->remoteAddress = remoteAddress;
}

//"reset" the send and recv threads so they will function correctly if we re-connect
void TCPClient::disconnectFromRemoteResponce() {
	clientConnectionRunning = false;
	if (this->networkThreadSend.joinable())
		this->networkThreadSend.join();
	
	if (this->networkThreadRecv.joinable())
		this->networkThreadRecv.join();
	int code = shutdown(this->clientSocketHandle, SD_BOTH);
	if (code != -1) {
		cout << "Disconnected From Remote Host" << endl;
	}
}

void TCPClient::disconnectFromRemote() {
	//clear the queues so this message goes out right away
	//We could also have implemented this using priority queues.  Which would have been super cool
	{unique_lock<mutex> guardSend(sendMutex);
	unique_lock<mutex> guardRecv(recvMutex);
	std::queue<wstring>().swap(toSend);
	std::queue<wstring>().swap(resolvedResponces);

	toSend.push(L"closeClientConnection"); }

	disconnectFromRemoteResponce();
}

void TCPClient::waitForResponceUI() {
	//Wait for items to be placed in the queue and then output once it's here
	while (clientConnectionRunning) {
		while (resolvedResponces.size() > 0) {
			unique_lock<mutex> guard(recvMutex);
			wstring responce = L"";
			responce = resolvedResponces.front();
				while (holdTheOutput) {}
				wcout << responce << endl;
				resolvedResponces.pop();
		}
	}
}


void TCPClient::getServerResponceOnThread() {
	constexpr long SIZE_OF_WCHAR = sizeof(wchar_t);

	//set the socket to non-blocking
	///this is done due to an issue where the thread will be stuck waiting for a recv connection on close

	while (clientConnectionRunning) {
		size_t sizeofString = 0;
		::recv(this->clientSocketHandle, (char*)&sizeofString, sizeof(sizeofString), 0);
		
		if (sizeofString > 0) {
			wstring results = L"";
			results.resize(sizeofString);

			int bytesRecieved = ::recv(this->clientSocketHandle, (char*)results.c_str(), SIZE_OF_WCHAR * (int)sizeofString, 0);
			if (bytesRecieved != -1) {
				addToResponceQueue(results);
			}	
		}
	}
	
}

void TCPClient::addToResponceQueue(std::wstring responceString) {
	unique_lock<mutex> guard(recvMutex);
	resolvedResponces.push(responceString);
}


//Thread that will always look to see if the enter key has been double pressed. This is used to allow the user to double hit enter then issue supplimentry commands
void TCPClient::waitForEnter() {
	DWORD cc;
	INPUT_RECORD irec;
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	
	
	while (clientConnectionRunning){
		ReadConsoleInput(h, &irec, 1, &cc);
		if (irec.EventType == KEY_EVENT && (irec.Event.KeyEvent.uChar.UnicodeChar == '\r')) {
			
			this->holdTheOutput = true;
			wcout << endl << endl;
			this_thread::sleep_for(chrono::seconds(10));
			this->holdTheOutput = false;
		}
	}	
}
