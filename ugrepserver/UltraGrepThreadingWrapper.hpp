#pragma once
#include "UltraGrepThreading.hpp"
#include "Cpp11Threads.hpp"
#include "Win32Threads.hpp"

/** @file UltraGrepThreadingWrapper.hpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Implemention that will adapt to the specified threading library

	I planned to have this look at the build version of C++ and auto switch to the "correct" version but this produced additional errors.
	Did not continue down this path, as per Garth.
*/

class UltraGrepThreadingWrapper : public UltraGrepThreading, private Cpp11Threads, private Win32Threads {
	bool isCpp11 = false;

public:
	UltraGrepThreadingWrapper(unsigned numberOfThreads, bool cpp11Thread, bool verbose);

	virtual void createThreadPool();
	virtual void run(Task initialTask);
	virtual std::wstring getResultSet();
	virtual void setVerboseMode(bool verboseMode);
	virtual void setServerReference(std::shared_ptr<TCPServer>& server);
};