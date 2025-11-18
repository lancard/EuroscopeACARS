#include "EuroscopeACARSHandler.h"
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#define RGB_YELLOW RGB(255, 255, 0)

std::string HttpGet(const std::string &url)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
	std::wstring wurl(wlen, 0);
	MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, &wurl[0], wlen);

	URL_COMPONENTS comp = {0};
	comp.dwStructSize = sizeof(comp);

	wchar_t host[256] = {};
	wchar_t path[2048] = {};
	comp.lpszHostName = host;
	comp.dwHostNameLength = 256;
	comp.lpszUrlPath = path;
	comp.dwUrlPathLength = 2048;

	if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &comp))
	{
		return "";
	}

	bool isHttps = (comp.nScheme == INTERNET_SCHEME_HTTPS);
	INTERNET_PORT port = comp.nPort;

	HINTERNET hSession = WinHttpOpen(L"HttpClient/1.0",
									 WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
									 WINHTTP_NO_PROXY_NAME,
									 WINHTTP_NO_PROXY_BYPASS, 0);

	if (!hSession)
		return "";

	HINTERNET hConnect = WinHttpConnect(hSession, comp.lpszHostName, port, 0);
	if (!hConnect)
	{
		WinHttpCloseHandle(hSession);
		return "";
	}

	HINTERNET hRequest = WinHttpOpenRequest(
		hConnect, L"GET", comp.lpszUrlPath,
		NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		isHttps ? WINHTTP_FLAG_SECURE : 0);

	if (!hRequest)
	{
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	BOOL result = WinHttpSendRequest(
		hRequest,
		WINHTTP_NO_ADDITIONAL_HEADERS,
		0,
		WINHTTP_NO_REQUEST_DATA, 0,
		0, 0);

	if (!result)
	{
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	result = WinHttpReceiveResponse(hRequest, NULL);
	if (!result)
	{
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	std::string response;
	DWORD sizeAvailable = 0;

	do
	{
		sizeAvailable = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &sizeAvailable))
			break;
		if (sizeAvailable == 0)
			break;

		std::string buffer(sizeAvailable, 0);
		DWORD bytesRead = 0;
		if (!WinHttpReadData(hRequest, &buffer[0], sizeAvailable, &bytesRead))
			break;

		response.append(buffer.c_str(), bytesRead);

	} while (sizeAvailable > 0);

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return response;
}

CEuroscopeACARSHandler::CEuroscopeACARSHandler(void) : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE,
															   "EuroscopeACARS",
															   "0.5",
															   "Sung-ho Kim",
															   "Sung-ho Kim")
{
	DisplayUserMessage("Message", "EuroscopeACARS", std::string("ACARS Loaded.").c_str(), false, false, false, false, false);

	if (this->GetLogonCode() == nullptr)
	{
		DisplayUserMessage("ACARS", "SYSTEM", "Please send your hoppie's logon code to access. ex) @hoppie (logoncode)", true, true, false, false, false);
	}
	else
	{
		DisplayUserMessage("ACARS", "SYSTEM", "Hoppie's logon code found. ACARS is ready to use.", true, true, false, false, false);
	}
}

CEuroscopeACARSHandler::~CEuroscopeACARSHandler(void)
{
	DisplayUserMessage("Message", "EuroscopeACARS", std::string("ACARS Unloaded.").c_str(), false, false, false, false, false);
}

void CEuroscopeACARSHandler::OnCompilePrivateChat(const char *sSenderCallsign,
												  const char *sReceiverCallsign,
												  const char *sChatMessage)
{
	// check starts with .hoppie
	std::string message(sChatMessage);
	if (message.rfind("@hoppie", 0) == 0)
	{
		// extract logon code
		std::string logonCode = message.substr(8); // skip "@hoppie "

		// save to settings
		this->SaveDataToSettings("LogonCode",
								 "Hoppie logon code for ACARS",
								 logonCode.c_str());

		// send confirmation message
		DisplayUserMessage("ACARS", "SYSTEM", "Hoppie's logon code saved successfully.", true, true, false, false, false);
	}
}

const char *CEuroscopeACARSHandler::GetLogonCode()
{
	const char *LogonCode = this->GetDataFromSettings("LogonCode");
	if (LogonCode == nullptr)
	{
		return nullptr;
	}

	return LogonCode;
}

void CEuroscopeACARSHandler::OnTimer(int Counter)
{
	char url[256];

	if (Counter % 30 == 0)
	{
		const char *LogonCode = this->GetLogonCode();
		if (LogonCode == nullptr)
			return;

		const char *MyCallsign = this->ControllerMyself().GetCallsign();
		// check callsign is empty
		if (MyCallsign[0] == '\0')
			return;

		// prepare url
		snprintf(url, 255, "http://www.hoppie.nl/acars/system/connect.html?logon=%s&from=%s&to=%s&type=poll&packet=",
				 LogonCode, MyCallsign, MyCallsign);
		std::string acars = HttpGet(url);
		DisplayUserMessage("ACARS", "ACARS", acars.c_str(), true, true, false, false, false);
	}
}

void CEuroscopeACARSHandler::OnFunctionCall(int FunctionId, const char *sItemString, POINT Pt, RECT Area)
{
}