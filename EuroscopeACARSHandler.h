#pragma once
#define PROGRAM_VERSION "1.1.1"
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
	bool printStarted = false;
	atomic<int> vatsimCid{0};
	atomic<const char *> vatsimCallsign{""};
	atomic<bool> terminateSignal{false};
	atomic<bool> isVatsimProductionServerConnected{false};
	thread hoppieWorkerThread;
	thread vatsimConnectionCheckThread;
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
	void HoppieThreadRunner();
	void VatsimConnectionCheckThreadRunner();

	const char *GetLogonCode();
	const char *GetLogonAddress();

	void DebugPrint(string message);
	void DisplayMessage(string catalog, string sender, string message);
	void ProcessMessage(string callsign, string message);
	void SendToHoppie(HoppieRequest h);

	int GlobalMessageId = 1000;
	unordered_map<string, string> LastMessageIdMap;
	unordered_map<string, string> PilotLogonMap;

	void OnCompilePrivateChat(const char *sSenderCallsign,
							  const char *sReceiverCallsign,
							  const char *sChatMessage);

	bool OnCompileCommand(const char *sCommandLine);
};
