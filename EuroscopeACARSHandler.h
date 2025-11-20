#pragma once
#include <SDKDDKVer.h>
#include <afxwin.h>
#include <string>
#include <unordered_map>
#include <format>
#include <sstream>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include "EuroScopePlugIn.h"

using namespace std;
using namespace EuroScopePlugIn;

class CEuroscopeACARSHandler : public EuroScopePlugIn::CPlugIn
{
public:
	CEuroscopeACARSHandler();
	~CEuroscopeACARSHandler();

	void OnFunctionCall(int FunctionId,
						const char *sItemString,
						POINT Pt,
						RECT Area);

	void OnTimer(int Counter);

	const char *GetLogonCode();
	const char *GetLogonAddress();

	void DebugPrint(string message);
	void ProcessMessage(string message);

	int GlobalMessageId = 1000;
	unordered_map<string, string> LastMessageIdMap;

	void OnCompilePrivateChat(const char *sSenderCallsign,
							  const char *sReceiverCallsign,
							  const char *sChatMessage);

	bool OnCompileCommand(const char *sCommandLine);
};
