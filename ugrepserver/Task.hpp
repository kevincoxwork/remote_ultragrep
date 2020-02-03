#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <regex>

/** @file Task.hpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Data Search Object
*/

class Task {
public:
	std::filesystem::path       fsFolderPath;
	std::vector<std::wstring>	  fileExtensions;
	std::wstring			   searchExpression;

	Task(std::filesystem::path  fp, std::vector<std::wstring> fe, std::wstring se);
	Task();
};