#pragma once
#include <SDKDDKVer.h>
#include <afxwin.h>
#include <string>
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

	std::string LastSender = "";
	std::string LastMessageId = "0";

	void OnCompilePrivateChat(const char *sSenderCallsign,
							  const char *sReceiverCallsign,
							  const char *sChatMessage);
};
