#include "Cpp11Threads.hpp"

/** @file cpp11threads.cpp
	@author Kevin Cox
	@date 2019-11-04
	@version 1.0.0
	@brief Cpp 11 Thread Search Impl
*/

using namespace std;

Cpp11Threads::Cpp11Threads(unsigned& numberOfThreads, bool& verbose) : nThreads(numberOfThreads), isVerbose(verbose) {
	this->barrierThreshold = this->nThreads + 1;
	this->barrierCount = this->barrierThreshold;
	this->barrierGeneration = 0;
	this->singleStart = LARGE_INTEGER();
	this->singleStop = LARGE_INTEGER();
	this->frequency = LARGE_INTEGER();
}

void Cpp11Threads::createCpp11ThreadPool() {
	threads.clear();
	for (unsigned i = 0; i < nThreads; ++i)
		threads.push_back(thread(&Cpp11Threads::performSearchTask, this));

	barrier();
}
void Cpp11Threads::barrier() {
	unique_lock<mutex> lock(barrierMtx);
	unsigned gen = barrierGeneration;
	if (--barrierCount == 0) {
		++barrierGeneration;
		barrierCount = barrierThreshold;
		barrierCond.notify_all();
	}
	else {
		while (gen == barrierGeneration)
			barrierCond.wait(lock);
	}
}

void Cpp11Threads::addCpp11SearchTask(Task& initialTask) {
	unique_lock<mutex> guard(taskMtx);
	tasks.push(initialTask);
	wakeCond.notify_one();
}

void Cpp11Threads::performSearchTask() {
	barrier();

	{ unique_lock<mutex> lk(wakeMtx);
	wakeCond.wait(lk); }
	{unique_lock<mutex> resultsLock(resultsMtx);
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&singleStart); }

	while (!this->tasks.empty()) {
		Task task;
		bool haveTask = false;
		std::filesystem::directory_iterator it = std::filesystem::directory_iterator();

		{ // DCLP - Double Check Lock Pattern
			unique_lock<mutex> taskLock(taskMtx);
			if (!this->tasks.empty()) {
				task = this->tasks.front();
				tasks.pop();
				try {
					it = std::filesystem::directory_iterator{ task.fsFolderPath };
				}
				catch (std::filesystem::filesystem_error)
				{
					if (isVerbose) {
						unique_lock<mutex> resultsLock(resultsMtx);
						wstring responce = L"No Access Perms To: " + task.fsFolderPath.wstring();
						this->serverReference->sendResponce(responce);
					}
				}
				haveTask = true;
			}
		}

		if (haveTask) {
			for (auto& p : it) {
				if (p.is_directory()) {
					{unique_lock<mutex> taskLock(taskMtx);
					tasks.push(Task(p.path(), task.fileExtensions, task.searchExpression)); }

					if (isVerbose) {
						unique_lock<mutex> resultsLock(resultsMtx);
						wstring responce = L"Scanning: " + p.path().wstring();
						this->serverReference->sendResponce(responce);
					}
				}
				else if (p.is_regular_file()) {
					//looks in these specified files for matching regex

					if (std::find(task.fileExtensions.begin(), task.fileExtensions.end(), (p.path().extension().wstring())) != task.fileExtensions.end()) {
						FILE* pFile;
						if (_wfopen_s(&pFile, p.path().wstring().c_str(), L"r")) {
							if (isVerbose) {
								unique_lock<mutex> resultsLock(resultsMtx);
								wstring responce = L"file: \"" + p.path().wstring() + L"\" does not exist.";
								this->serverReference->sendResponce(responce);
							}
						}

						if (isVerbose) {
							unique_lock<mutex> resultsLock(resultsMtx);
							wstring responce = L"Grepping: " + p.path().wstring();
							this->serverReference->sendResponce(responce);
						}

						wchar_t currentLine[1024];

						unsigned lineNumber = 1;
						wsmatch wideMatch;
						wregex wideRegexExpression(task.searchExpression);
						//Get Each Line, 1024 WChars Long as a max

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
								unique_lock<mutex> taskLock(resultsMtx);
								items.push_back(L"[" + to_wstring(lineNumber) + L":" + to_wstring(currentNumberOfMatches) + L"]" + currentLine);

								if (isVerbose) {
									wstring responce = L"Matched " + to_wstring(currentNumberOfMatches) + L":   \"" + p.path().wstring() + L"\""  L" -  [" + to_wstring(lineNumber) + L"]" + L" " + currentLine;
									this->serverReference->sendResponce(responce);
								}
							}
							else if (currentNumberOfMatches == 1) {
								unique_lock<mutex> taskLock(resultsMtx);
								items.push_back(L"[" + to_wstring(lineNumber) + L"]" + currentLine);

								if (isVerbose) {
									wstring responce = L"Matched " + to_wstring(currentNumberOfMatches) + L":   \"" + p.path().wstring() + L"\""  L" -  [" + to_wstring(lineNumber) + L"]" + L" " + currentLine;
									this->serverReference->sendResponce(responce);
								}
							}

							currentNumberOfMatches = 0;
							lineNumber++;
						}

						if (totalNumberOfMatches > 0) {
							unique_lock<mutex> resultsLock(resultsMtx);
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
	//get total time for report
	{unique_lock<mutex> resultsLock(resultsMtx);
	QueryPerformanceCounter(&singleStop);
	this->resultSet.scanCompletedIn = (singleStop.QuadPart - singleStart.QuadPart) / double(frequency.QuadPart); }
}

std::wstring Cpp11Threads::getCpp11ResultSet() {
	waitForThreads();
	wstring output = L"\n\nGrep Report\n";
	for (size_t i = 0; i < resultSet.finalGREPReport.size(); i++) {
		output += L"\"" + resultSet.finalGREPReport[i].path + L"\"" + L"\n";

		for (size_t x = 0; x < resultSet.finalGREPReport[i].items.size(); x++) {
			output += resultSet.finalGREPReport[i].items[x];
		}
	}
	output += L"\n\n";

	output += L"Files with matches = " + to_wstring(resultSet.filesMatched) + L"\n";
	output += L"Total number of matches = " + to_wstring(resultSet.totalMatches) + L"\n";
	output += L"Scan completed in " + to_wstring(resultSet.scanCompletedIn) + L"\n\n";

	return output;
}

void Cpp11Threads::waitForThreads() {
	// cleanup
	wakeCond.notify_all();

	for (auto& t : threads)
		t.join();
}

void Cpp11Threads::setVerboseModeCpp11(bool verboseMode) {
	this->isVerbose = verboseMode;
}

void Cpp11Threads::setCpp11TCPServerReference(shared_ptr<TCPServer>& server) {
	this->serverReference = server;
}