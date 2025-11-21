#pragma once
#define PROGRAM_VERSION "1.0.2"
#include <SDKDDKVer.h>
#include <afxwin.h>
#include <thread>
#include <string>
#include <ranges>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <format>
#include <sstream>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include "EuroScopePlugIn.h"

using namespace std;
using namespace EuroScopePlugIn;

#include "ConcurrentQueue.h"
#include "HoppieRequestAndResponse.h"

class CEuroscopeACARSHandler : public EuroScopePlugIn::CPlugIn
{
private:
	atomic<bool> terminateSignal{false};
	thread workerThread;
	ConcurrentQueue<HoppieRequest> requestQueue;
	ConcurrentQueue<HoppieResponse> responseQueue;

public:
	CEuroscopeACARSHandler();
	~CEuroscopeACARSHandler();

	void OnFunctionCall(int FunctionId,
						const char *sItemString,
						POINT Pt,
						RECT Area);

	void OnTimer(int Counter);
	void OnTimerProcessResponse();
	void OnTimerRequestPolling();
	void ThreadRunner();

	const char *GetLogonCode();
	const char *GetLogonAddress();

	void DebugPrint(string message);
	void ProcessMessage(string callsign, string message);
	void SendToHoppie(HoppieRequest h);

	int GlobalMessageId = 1000;
	unordered_map<string, string> LastMessageIdMap;

	void OnCompilePrivateChat(const char *sSenderCallsign,
							  const char *sReceiverCallsign,
							  const char *sChatMessage);

	bool OnCompileCommand(const char *sCommandLine);
};
