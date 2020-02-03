#pragma once
#define _WINSOCKAPI_
#include "TCPServer.hpp"
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include "Task.hpp"
#include "ResultSet.hpp"
#include <Windows.h>

/** @file cpp11threads.hpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Cpp 11 Thread Search
*/

class Cpp11Threads {
public:

	//Thread Member Variables
	unsigned nThreads;

	//Barrier Member Variables
	unsigned barrierThreshold;
	unsigned barrierCount;
	unsigned barrierGeneration;

	//Events
	std::condition_variable barrierCond;
	std::condition_variable wakeCond;

	//Critical Sections
	std::mutex barrierMtx;
	std::mutex wakeMtx;
	std::mutex taskMtx;
	std::mutex resultsMtx;

	std::queue<Task> tasks;
	std::vector<std::thread> threads;

	bool isVerbose;

	//Timing
	LARGE_INTEGER singleStart, singleStop, frequency;

	std::shared_ptr<TCPServer> serverReference;

public:
	//1 Arg - Construtor
	Cpp11Threads(unsigned& numberOfThreads, bool& verbose);

	void createCpp11ThreadPool();
	void barrier();
	void addCpp11SearchTask(Task& initialTask);
	void performSearchTask();
	void waitForThreads();

	//Report Info
	ResultSet resultSet;

	std::wstring getCpp11ResultSet();
	void setVerboseModeCpp11(bool verboseMode);

	void setCpp11TCPServerReference(std::shared_ptr<TCPServer>& server);
};