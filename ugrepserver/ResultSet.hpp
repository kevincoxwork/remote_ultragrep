#pragma once
#include <vector>
#include <string>
/** @file ResultSet.hpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Verbose and Regular Report Saved
*/

class GrepItem {
public:
	std::wstring path = L"";
	std::vector<std::wstring> items;
	int numberOfMatches = 0;
};

class ResultSet {
public:
	int filesMatched = 0;
	int totalMatches = 0;
	double scanCompletedIn = 0;
	std::vector<GrepItem> finalGREPReport;
};