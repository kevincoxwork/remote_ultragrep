#pragma once
#include "Task.hpp"
#include "ResultSet.hpp"
#include <memory>
#include "TCPServer.hpp"

/** @file UltraGrepThreading.hpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Parent Interface for Adapter
*/

class UltraGrepThreading {
public:
	virtual void createThreadPool() = 0;
	virtual void run(Task initialTask) = 0;
	virtual std::wstring getResultSet() = 0;
	virtual void setVerboseMode(bool verboseMode) = 0;
	virtual void setServerReference(std::shared_ptr<TCPServer>& server) = 0;
};