#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include "Task.hpp"
#include <regex>
#include "CriticalSection.hpp"
#include "ResultSet.hpp"
#include "TCPServer.hpp"

/** @file Win32Threads.hpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Win32 Api Thread Search
*/

class Win32Threads {
	//Thread Member Variables
	unsigned nThreads;
	std::vector<HANDLE> threads;

	//Barrier Member Variables
	unsigned barrierThreshold;
	unsigned barrierCount;
	unsigned barrierGeneration;

	//Events
	HANDLE barrierEvent;
	HANDLE wakeEvent;

	//Critical Sections
	CriticalSection taskMtx;
	CriticalSection resultsMtx;

	std::queue<Task> tasks;

	bool isVerbose;

	LARGE_INTEGER singleStart, singleStop, frequency;

	std::shared_ptr<TCPServer> serverReference;

public:
	Win32Threads(unsigned& numberOfThreads, bool& verbose);

	void createWin32ThreadPool();
	void barrier();

	void addWin32SearchTask(Task& initialTask);

	DWORD performSearchTask();
	void waitForThreads();

	static DWORD WINAPI performSearchTaskCaller(LPVOID thisClass);

	ResultSet resultSet;

	std::wstring getWin32ResultSet();
	void setVerboseModeWin32(bool verboseMode);

	void setWin32TCPServerReference(std::shared_ptr<TCPServer>& server);
};