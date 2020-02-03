#include "Task.hpp"
/** @file Task.cpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Data Search Object impl
*/

Task::Task(std::filesystem::path  fp, std::vector<std::wstring> fe, std::wstring se) : fsFolderPath(fp), fileExtensions(fe), searchExpression(se) {}

Task::Task() {
};