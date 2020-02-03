#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <regex>
#include <filesystem>
#include <boost/algorithm/string/replace.hpp>

/** @file UltraGrepSearch.hpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Data Parsing and Holding Object
*/

class UltraGrepSearch {
	bool                       verboseFlag;
	std::wstring			   folderPath;
	std::wstring			   searchExpression;
	std::vector<std::wstring>  fileExtensions;

	//We are using the boost filesystem instead of the c++ 11 filesystem as to make the code portable to support versions older than c++17
	std::filesystem::path       fsFolderPath;
public:
	UltraGrepSearch(bool& vf, std::wstring& fp, std::wstring& se);

	void delimitFileExtensions(std::wstring& fileList);

	bool doesFolderPathExist();
	bool isValidRegex();

	bool getVerboseFlag();

	std::filesystem::path getfsFolderPath();
	std::wstring getSeatchExpression();
	std::vector<std::wstring>  getFileExtensions();

	std::wstring to_wstring();
};