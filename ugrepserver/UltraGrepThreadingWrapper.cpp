#pragma once
#include "UltraGrepThreadingWrapper.hpp"

/** @file UltraGrepThreadingWrapper.cpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Implemention that will adapt to the specified threading library

	I planned to have this look at the build version of C++ and auto switch to the "correct" version but this produced additional errors.
	Did not continue down this path, as per Garth.
*/

UltraGrepThreadingWrapper::UltraGrepThreadingWrapper(unsigned numberOfThreads, bool cpp11Thread, bool verbose) : Win32Threads(numberOfThreads, verbose), Cpp11Threads(numberOfThreads, verbose), isCpp11(cpp11Thread) {
}

void UltraGrepThreadingWrapper::createThreadPool()
{
	//run the cpp11 or the win32 version based on the build type
	if (isCpp11) {
		this->createCpp11ThreadPool();
	}
	else {
		this->createWin32ThreadPool();
	}
}

void UltraGrepThreadingWrapper::run(Task initialTask) {
	if (isCpp11) {
		this->addCpp11SearchTask(initialTask);
	}
	else {
		this->addWin32SearchTask(initialTask);
	}
}

std::wstring UltraGrepThreadingWrapper::getResultSet() {
	if (isCpp11) {
		return this->getCpp11ResultSet();
	}
	else {
		return this->getWin32ResultSet();
	}
}

void UltraGrepThreadingWrapper::setVerboseMode(bool verboseMode) {
	if (isCpp11) {
		this->setVerboseModeCpp11(verboseMode);
	}
	else {
		this->setVerboseModeWin32(verboseMode);
	}
}

void UltraGrepThreadingWrapper::setServerReference(std::shared_ptr<TCPServer>& server) {
	if (isCpp11)
		this->setCpp11TCPServerReference(server);
	else
		this->setWin32TCPServerReference(server);
}