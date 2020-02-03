#include "Win32Threads.hpp"

/** @file Win32Threads.cpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Win32 Api Thread Search Impl
*/

using namespace std;

void Win32Threads::createWin32ThreadPool() {
	this->wakeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->barrierEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// launch worker threads.
	for (unsigned i = 0; i < nThreads; ++i)
		threads.push_back(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)performSearchTaskCaller, this, 0, NULL));

	this->barrier();
}

DWORD WINAPI Win32Threads::performSearchTaskCaller(LPVOID thisClass) {
	Win32Threads* instance = reinterpret_cast<Win32Threads*>(thisClass);
	return instance->performSearchTask();
}

void Win32Threads::addWin32SearchTask(Task& initialTask) {
	{ CSLock lk(taskMtx);
	tasks.push(initialTask); }
	SetEvent(wakeEvent);
}

DWORD Win32Threads::performSearchTask() {
	this->barrier();

	WaitForSingleObject(wakeEvent, INFINITE);

	{ CSLock lk(resultsMtx);
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&singleStart); }

	while (!this->tasks.empty()) {
		Task task;
		bool haveTask = false;
		std::filesystem::directory_iterator it = std::filesystem::directory_iterator();

		{ // DCLP - Double Check Lock Pattern
			CSLock lk(taskMtx);
			if (!this->tasks.empty()) {
				task = this->tasks.front();
				tasks.pop();
				try {
					it = std::filesystem::directory_iterator{ task.fsFolderPath };
				}
				catch (std::filesystem::filesystem_error)
				{
					if (this->isVerbose) {
						CSLock lk(resultsMtx);
						wstring responce = L"No Access Perms To: " + task.fsFolderPath.wstring() + L"\n";
						this->serverReference->sendResponce(responce);
					}
				}
				haveTask = true;
			}
		}

		if (haveTask) {
			for (auto& p : it) {
				if (p.is_directory()) {
					{ CSLock lk(taskMtx);
					tasks.push(Task(p.path(), task.fileExtensions, task.searchExpression));
					}

					if (isVerbose) {
						wstring responce = L"Scanning: " + p.path().wstring() + L"\n";
						this->serverReference->sendResponce(responce);
					}
				}
				else if (p.is_regular_file()) {
					//looks in these specified files for matching regex
					if (std::find(task.fileExtensions.begin(), task.fileExtensions.end(), (p.path().extension().wstring())) != task.fileExtensions.end()) {
						FILE* pFile;
						if (_wfopen_s(&pFile, p.path().wstring().c_str(), L"r")) {
							if (isVerbose) {
								CSLock lk(resultsMtx);
								wstring responce = L"file: \"" + p.path().wstring() + L"\" does not exist.\n";
								this->serverReference->sendResponce(responce);
							}
						}

						if (isVerbose) {
							CSLock lk(resultsMtx);
							wstring responce = L"Grepping: " + p.path().wstring() + L"\n";
							this->serverReference->sendResponce(responce);
						}

						wchar_t currentLine[1024];

						unsigned lineNumber = 1;
						wsmatch wideMatch;
						wregex wideRegexExpression(task.searchExpression);

						//Get Each Line, 250 WChars Long as a max
						int totalNumberOfMatches = 0;
						int currentNumberOfMatches = 0;

						//fgetWs was asking for this to be checked
						if (pFile == 0) {
							break;
						}

						vector<wstring> items;
						while (fgetws(currentLine, 1024, pFile) != NULL)
						{
							wstring lineTemp = wstring(currentLine);

							while (regex_search(lineTemp, wideMatch, wideRegexExpression)) {
								//take into consideration of multiple matches per mine
								currentNumberOfMatches++;
								totalNumberOfMatches++;
								lineTemp = wideMatch.suffix();
							}
							if (currentNumberOfMatches > 1) {
								CSLock lk(resultsMtx);
								items.push_back(L"[" + to_wstring(lineNumber) + L":" + to_wstring(currentNumberOfMatches) + L"]" + currentLine);
								if (isVerbose) {
									wstring responce = L"Matched " + to_wstring(currentNumberOfMatches) + L":   \"" + p.path().wstring() + L"\""  L" -  [" + to_wstring(lineNumber) + L"]" + L" " + currentLine + L"\n";
									this->serverReference->sendResponce(responce);
								}
							}
							else if (currentNumberOfMatches == 1) {
								CSLock lk(resultsMtx);
								items.push_back(L"[" + to_wstring(lineNumber) + L"]" + currentLine);

								if (isVerbose) {
									wstring responce = L"Matched " + to_wstring(currentNumberOfMatches) + L":   \"" + p.path().wstring() + L"\""  L" -  [" + to_wstring(lineNumber) + L"]" + L" " + currentLine + L"\n";
									this->serverReference->sendResponce(responce);
								}
							}

							currentNumberOfMatches = 0;
							lineNumber++;
						}

						if (totalNumberOfMatches > 0) {
							CSLock lk(resultsMtx);
							this->resultSet.filesMatched++;
							this->resultSet.totalMatches += totalNumberOfMatches;

							GrepItem item;
							item.numberOfMatches = currentNumberOfMatches;
							item.path = p.path().wstring();
							item.items = items;
							this->resultSet.finalGREPReport.push_back(item);
						}
					}
				}
			}
		}
	}

	{CSLock lk(resultsMtx);
	QueryPerformanceCounter(&singleStop);
	this->resultSet.scanCompletedIn = (singleStop.QuadPart - singleStart.QuadPart) / double(frequency.QuadPart); }

	return 0;
}

Win32Threads::Win32Threads(unsigned& numberOfThreads, bool& verbose) {
	//Initialize Member Variables
	this->nThreads = numberOfThreads;
	this->isVerbose = verbose;

	//Initialize Barrier Member Variables
	this->barrierThreshold = this->nThreads + 1;
	this->barrierCount = this->barrierThreshold;
	this->barrierGeneration = 0;
	this->singleStart = LARGE_INTEGER();
	this->singleStop = LARGE_INTEGER();
	this->frequency = LARGE_INTEGER();

	this->wakeEvent = NULL;
	this->barrierEvent = NULL;
}

void Win32Threads::barrier() {
	unsigned gen = barrierGeneration;
	if (--barrierCount == 0) {
		++barrierGeneration;
		barrierCount = barrierThreshold;
		SetEvent(barrierEvent);
	}
	else {
		while (gen == barrierGeneration)
			WaitForSingleObject(barrierEvent, INFINITE);
	}
}

std::wstring Win32Threads::getWin32ResultSet() {
	waitForThreads();

	wstring output = L"\nGrep Report\n";
	for (size_t i = 0; i < resultSet.finalGREPReport.size(); i++) {
		output += L"\"" + resultSet.finalGREPReport[i].path + L"\"" + L"\n";

		for (size_t x = 0; x < resultSet.finalGREPReport[i].items.size(); x++) {
			output += resultSet.finalGREPReport[i].items[x];
		}
	}
	output += L"\n\n";

	output += L"Files with matches = " + to_wstring(resultSet.filesMatched) + L"\n";
	output += L"Total number of matches = " + to_wstring(resultSet.totalMatches) + L"\n";
	output += L"Scan completed in " + to_wstring(resultSet.scanCompletedIn) + L"\n";

	return output;
}

void Win32Threads::waitForThreads() {
	for (size_t i = 0; i < nThreads; ++i)
		SetEvent(wakeEvent);

	// cleanup
	WaitForMultipleObjects((DWORD)threads.size(), threads.data(), TRUE, INFINITE);
	for (auto& t : threads)
		CloseHandle(t);

	CloseHandle(barrierEvent);
	CloseHandle(wakeEvent);
}

void Win32Threads::setVerboseModeWin32(bool verboseMode) {
	this->isVerbose = verboseMode;
}

void Win32Threads::setWin32TCPServerReference(std::shared_ptr<TCPServer>& server) {
	this->serverReference = server;
}