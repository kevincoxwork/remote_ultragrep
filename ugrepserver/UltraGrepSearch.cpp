#include "UltraGrepSearch.hpp"

/** @file UltraGrepSearch.cpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Data Parsing and Holding Object Impl
*/

using namespace std;

UltraGrepSearch::UltraGrepSearch(bool& vf, std::wstring& fp, std::wstring& se) : verboseFlag(vf), folderPath(fp), searchExpression(se) {
	this->fileExtensions = vector<wstring>();
}

void UltraGrepSearch::delimitFileExtensions(wstring& fileList) {
	wstring currentFileExtension;
	wstringstream stringStream(fileList);

	while (getline(stringStream, currentFileExtension, L'.')) {
		if (currentFileExtension != L"")
			this->fileExtensions.push_back(L"." + currentFileExtension);
	}
}

wstring UltraGrepSearch::to_wstring() {
	wstring fileExtensionsString = L"";
	wstring flag = L"false";

	if (this->verboseFlag)
		flag = L"true";

	for (size_t i = 0; i < fileExtensions.size(); i++) {
		fileExtensionsString += fileExtensions[i] + L",";
	}
	fileExtensionsString = fileExtensionsString.substr(0, fileExtensionsString.size() - 1);

	return wstring(L"Verbose: " + flag + L"\nFolderPath: " + this->folderPath + L"\nSearchExpression: " + this->searchExpression + L"\nFileExtensions: " + fileExtensionsString + L"\n");
}

bool UltraGrepSearch::doesFolderPathExist() {
	boost::replace_all(folderPath, "\"", "");
	
	if (filesystem::exists(this->folderPath)) {
		//I would use the u8Path function here but it seems it will be deprecated in C++2020
		//https://en.cppreference.com/w/cpp/filesystem/path

		this->fsFolderPath = filesystem::path(this->folderPath);
		return true;
	}
	return false;
}

bool UltraGrepSearch::isValidRegex() {
	return true;
}

bool UltraGrepSearch::getVerboseFlag() {
	return this->verboseFlag;
}

std::filesystem::path UltraGrepSearch::getfsFolderPath() {
	return this->fsFolderPath;
}
std::wstring UltraGrepSearch::getSeatchExpression() {
	return this->searchExpression;
}
std::vector<std::wstring> UltraGrepSearch::getFileExtensions() {
	return this->fileExtensions;
}