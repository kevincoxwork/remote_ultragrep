/** @file ultra_grep_client_main.cpp
	@author Kevin Cox
	@date 2019-10-21
	@version 1.0.0
	@brief Ultra Grep Client Main
*/
#include <SDKDDKVer.h>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <boost/asio/ip/address.hpp>
#include "TCPClient.hpp"

using namespace std;

enum class ISSUED_COMMAND_TYPE
{
	GREP_COMMAND,
	DROP_COMMAND,
	CONNECT_COMMAND,
	STOP_SERVER_COMMAND,
	INVALID_COMMAND,
	PRE_ISSUED_CONNECT,
	CLOSE_COMMAND
};
//map command responce to enum
ISSUED_COMMAND_TYPE resolveCommandAction(wstring& commandLine) {
	if (commandLine == L"grep")
		return ISSUED_COMMAND_TYPE::GREP_COMMAND;
	else if (commandLine == L"drop")
		return ISSUED_COMMAND_TYPE::DROP_COMMAND;
	else if (commandLine == L"connect")
		return ISSUED_COMMAND_TYPE::CONNECT_COMMAND;
	else if (commandLine == L"stopserver")
		return ISSUED_COMMAND_TYPE::STOP_SERVER_COMMAND;
	else if (commandLine == L"close")
		return ISSUED_COMMAND_TYPE::CLOSE_COMMAND;
	else
		return ISSUED_COMMAND_TYPE::INVALID_COMMAND;
}
//wait for valid input
vector<wstring> promptForCommand() {
	wstring commandLineInput = L"";
	wstring commandLineInputOrigin = L"";

	std::getline(std::wcin, commandLineInputOrigin);
	std::wstringstream ss(commandLineInputOrigin);

	vector<wstring> commands;
	while (getline(ss, commandLineInput, L' ')) {
		commands.push_back(commandLineInput);
	}
	return commands;
}

//verify that the client is connected to a server instance
bool isConnected(bool connected) {
	if (!connected) {
		cout << "Not Connected To Remote Host: Please Connect Before Issuing This Command" << endl;
		return false;
	}
	return true;
}
//verify valid ip
bool isValidIPAddress(string& serverAddressRaw, boost::asio::ip::address_v4& serverAddressIP) {
	try {
		serverAddressIP = serverAddressIP.from_string(serverAddressRaw);
	}
	catch (boost::system::system_error ex) {
		return false;
	}
	return true;
}
//verify valid ip overload for wstring
bool isValidIPAddress(wstring& serverAddressRaw, boost::asio::ip::address_v4& serverAddressIP) {
	try {
		string reguarStringIP;
		for (int i = 0; i < serverAddressRaw.size(); i++) {
			reguarStringIP += (char)serverAddressRaw.at(i);
		}
		serverAddressIP = serverAddressIP.from_string(reguarStringIP);
	}
	catch (boost::system::system_error ex) {
		return false;
	}
	return true;
}

//Main Method for Ultra_Grep Client
//Arg 1 - Server IP
int main(int argc, char* argv[]) {
	ISSUED_COMMAND_TYPE commandAction = ISSUED_COMMAND_TYPE::INVALID_COMMAND;
	boost::asio::ip::address_v4 serverAddressIP;
	string serverAddressRaw = "";
	const unsigned short REMOTE_SERVER_PORT_NUMBER = 25565;

	cout << "UltraGrep (v1.0): Kevin Cox" << endl;
	//if valid number of args
	if (argc >= 2) {
		for (int i = 1; i < argc; i++) {
			//test to see if its the verbose flag
			if (serverAddressRaw._Equal("")) {
				istringstream iss(argv[i]);
				iss >> serverAddressRaw;
				if (!iss) {
					cout << L"Bad Character In Server Address" << endl;
					return EXIT_FAILURE;
				}
			}
		}

		//Check if the passed IP is provided
		if (serverAddressRaw == "") {
			cout << "Usage: ugrepclient [ip]" << endl;
			return EXIT_FAILURE;
		}
		//Check if the passed IP is valid format
		if (!isValidIPAddress(serverAddressRaw, serverAddressIP)) {
			cout << "Invalid IP Format In Server Address" << endl;
			return EXIT_FAILURE;
		}

		commandAction = ISSUED_COMMAND_TYPE::PRE_ISSUED_CONNECT;
	}

	bool exitCommandLoop = true;
	bool connectedState = false;

	TCPClient client;

	while (exitCommandLoop) {
		vector<wstring> commands;

		if (commandAction == ISSUED_COMMAND_TYPE::PRE_ISSUED_CONNECT) {
			commandAction = ISSUED_COMMAND_TYPE::CONNECT_COMMAND;
		}
		else {
			commands = promptForCommand();
			if (commands.size() == 0)
				continue;
			commandAction = resolveCommandAction(commands.front());
		}
		//Command is grep - send grep request
		if (commandAction == ISSUED_COMMAND_TYPE::GREP_COMMAND) {
			if (isConnected(connectedState)) {
				client.sendRequest(commands);
			}
		}
		//command is drop - drop from server and reset connect state
		else if (commandAction == ISSUED_COMMAND_TYPE::DROP_COMMAND) {
			if (isConnected(connectedState)) {
				client.disconnectFromRemote();
				connectedState = false;
				serverAddressIP = serverAddressIP.from_string("0.0.0.0");
			}
		}
		//connect to remote instance
		else if (commandAction == ISSUED_COMMAND_TYPE::CONNECT_COMMAND) {
			if (connectedState) {
				client.addToResponceQueue(L"You Are Already Connected To A Remote Instance\n");
			}
			else {
				if (serverAddressIP.to_string() == "0.0.0.0") {
					//prompt for connect info
					//Check if the passed IP is valid format
					if (commands.size() < 2) {
						client.addToResponceQueue(L"Please Provide Host Address In The Command\n");
						commandAction = ISSUED_COMMAND_TYPE::INVALID_COMMAND;
						continue;
					}
					if (!isValidIPAddress(commands[1], serverAddressIP)) {
						client.addToResponceQueue(L"Invalid IP Format In Server Address\n");
						serverAddressIP = boost::asio::ip::address_v4();
						commandAction = ISSUED_COMMAND_TYPE::INVALID_COMMAND;
						continue;
					}
				}

				client.setRemoteAddress(serverAddressIP.to_string());
				client.setRemotePortNumber(REMOTE_SERVER_PORT_NUMBER);

				if (!client.connectToRemote()) {
					string stringIP = serverAddressIP.to_string();
					client.addToResponceQueue(L"Error: Unable To Connect To Remote Host on " + wstring(stringIP.begin(), stringIP.end()) + L":" + to_wstring(REMOTE_SERVER_PORT_NUMBER) + L"\n");
					connectedState = false;
					serverAddressIP = boost::asio::ip::address_v4();
					commandAction = ISSUED_COMMAND_TYPE::INVALID_COMMAND;
					continue;
				}
				string stringIP = serverAddressIP.to_string();
				client.addToResponceQueue(L"Connected To Remote Host: " + wstring(stringIP.begin(), stringIP.end()) + L":" + to_wstring(REMOTE_SERVER_PORT_NUMBER) + L"\n");
				connectedState = true;
			}
		}
		//stop remote server command
		else if (commandAction == ISSUED_COMMAND_TYPE::STOP_SERVER_COMMAND) {
			if (isConnected(connectedState)) {
				client.stopRemoteServer();
				connectedState = false;
				serverAddressIP = serverAddressIP.from_string("0.0.0.0");
			}
		}
		//personal added command to close cmd window.
		else if (commandAction == ISSUED_COMMAND_TYPE::CLOSE_COMMAND) {
			exitCommandLoop = false;
		}
		else {
			client.addToResponceQueue(L"Invalid Command Entered");
		}
	}

	return EXIT_SUCCESS;
}