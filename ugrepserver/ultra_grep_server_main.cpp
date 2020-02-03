/** @file ultra_grep_server_main.cpp
	@author Kevin Cox
	@date 2019-11-15
	@version 1.0.0
	@brief Ultra Grep Server Main
*/
#include <SDKDDKVer.h>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <boost/asio/ip/address.hpp>
#include "UltraGrepSearch.hpp"
#include "UltraGrepThreadingWrapper.hpp"
#include "TCPServer.hpp"

using namespace std;

//Main Method for Ultra_Grep
//Arg 1 - Specified IP Adress of Host System

bool isValidIPAddress(string& hostBindingRaw, boost::asio::ip::address_v4& addressBindingIP) {
	try {
		addressBindingIP = addressBindingIP.from_string(hostBindingRaw);
	}
	catch (boost::system::system_error ex) {
		return false;
	}

	return true;
}

UltraGrepSearch mapCommandsToArgs(vector<wstring>& commandsRecieved, wstring& errorResult, bool& isErrorBool) {
	wstring folderPath = L"";
	bool verboseFlag = false;
	wstring searchExpression = L"";
	wstring fileExtensions = L".txt";

	wstring verboseFlagStringData;

	if (commandsRecieved.size() == 1) {
		errorResult = L"Usage: grep [-v] folder regex [.ext]*";
		isErrorBool = true;
		return UltraGrepSearch(verboseFlag, folderPath, searchExpression);
	}

	for (size_t i = 1; i < commandsRecieved.size(); i++) {
		//test to see if its the verbose flag
		if (verboseFlagStringData._Equal(L"")) {
			verboseFlagStringData = commandsRecieved[i];
			if (verboseFlagStringData._Equal(L"-v")) {
				verboseFlag = true;
			}
			else {
				i--;
				continue;
			}
		}
		else if (folderPath._Equal(L"")) {
			folderPath = commandsRecieved[i];
		}
		else if (searchExpression._Equal(L"")) {
			searchExpression = commandsRecieved[i];
		}
		else if (fileExtensions._Equal(L".txt")) {
			fileExtensions = commandsRecieved[i];
		}
	}
	if (searchExpression._Equal(L"")) {
		errorResult = L"No Regex Search Expression";
		isErrorBool = true;
	}
	UltraGrepSearch searchObject = UltraGrepSearch(verboseFlag, folderPath, searchExpression);
	searchObject.delimitFileExtensions(fileExtensions);
	//verify path
	if (!searchObject.doesFolderPathExist()) {
		errorResult = +L"Error: folder<" + folderPath + L"> doesn't exist.";
		isErrorBool = true;
	}
	return searchObject;
}
//We are not using wMain here since the only parameter cannot have unicode chars
int main(int argc, char* argv[]) {
	const unsigned NUMBER_OF_THREADS = 4;
	const unsigned short SERVER_PORT_NUMBER = 25565;
	const bool useCpp11Theading = TRUE;
	const bool initialVerboseMode = FALSE;
	const string LOCALHOST_BINDING = "127.0.0.1";
	boost::asio::ip::address_v4 addressBindingIP = addressBindingIP.from_string(LOCALHOST_BINDING);
	string hostBindingRaw = "127.0.0.1";

	cout << "UltraGrep Server (v1.0): Kevin Cox" << endl;
	for (int i = 1; i < argc; i++) {
		istringstream iss(argv[i]);
		iss >> hostBindingRaw;
		if (!iss) {
			cout << "Bad Character In Host Binding" << endl;
			return EXIT_FAILURE;
		}
	}

	//Check if the passed IP is valid
	if (hostBindingRaw != LOCALHOST_BINDING) {
		if (!isValidIPAddress(hostBindingRaw, addressBindingIP)) {
			cout << "Invalid IP Adress" << endl;
			return EXIT_FAILURE;
		}
	}
	//had to change the server impl so that we can create messages from inside the grepthreading class. This was needed to support verbose mode as per specs
	//We are passing a shared pointer into the inner impl of grepThreading
	shared_ptr<TCPServer> server = make_shared<TCPServer>(SERVER_PORT_NUMBER, addressBindingIP.to_string());
	unique_ptr<UltraGrepThreading> grepThreading = make_unique<UltraGrepThreadingWrapper>(NUMBER_OF_THREADS, useCpp11Theading, initialVerboseMode);
	grepThreading->setServerReference(server);
	grepThreading->createThreadPool();

	//We can start up the server now
	server->start();

	bool serverLoop = true;

	while (serverLoop) {
		vector<wstring> commandsRecieved = server->waitForCommand();
		if (commandsRecieved.size() == 0) {
			//TODO
			//respond with error
			wstring repsonce = L"Error: Invalid Command Sent To Server";
			server->sendResponce(repsonce);
		}
		if (commandsRecieved[0] == L"grep") {
			wstring errorResult = L"";
			bool isErrorBool = false;

			//if passes, we have valid commands and can then search
			UltraGrepSearch searchObject = mapCommandsToArgs(commandsRecieved, errorResult, isErrorBool);

			if (isErrorBool) {
				//TODO
				//respond with error
				server->sendResponce(errorResult);
				continue;
			}
			//verbose flag may change from run to run. had to add an inner flag switch
			if (searchObject.getVerboseFlag())
				grepThreading->setVerboseMode(searchObject.getVerboseFlag());

			//perform search and pass back to client
			grepThreading->run(Task(searchObject.getfsFolderPath(), searchObject.getFileExtensions(), searchObject.getSeatchExpression()));
			wstring results = grepThreading->getResultSet();
			server->sendResponce(results);
			//reset the connection for another grep command
			grepThreading->createThreadPool();
		}
		else if (commandsRecieved[0] == L"stopserver") {
			server->stop();
			serverLoop = false;
		}
		else if (commandsRecieved[0] == L"closeClientConnection") {
			server->closeClientConnection();
		}
	}

	return EXIT_SUCCESS;
}