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

std::string ConvertCpdlcHttpEncode(const std::string &value)
{
	std::string escaped = "";

	for (char c : value)
	{
		if (c == ' ')
		{
			escaped += '+';
			continue;
		}

		if (c >= 'a' && c <= 'z')
			c -= 32; // 'a'->'A'

		if (isalnum(static_cast<unsigned char>(c)) ||
			c == '-' || c == '_' || c == '.' || c == '@')
		{
			escaped += c;
		}
	}
	return escaped;
}

CEuroscopeACARSHandler::CEuroscopeACARSHandler(void) : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE,
															   "EuroscopeACARS",
															   "0.9",
															   "Sung-ho Kim",
															   "Sung-ho Kim")
{
	DisplayUserMessage("Message", "EuroscopeACARS", std::string("ACARS Loaded.").c_str(), false, false, false, false, false);

	if (GetLogonCode() == nullptr)
	{
		DisplayUserMessage("ACARS", "SYSTEM", "Please send your hoppie's logon code to access. ex) @hoppie (logoncode)", true, true, false, false, false);
	}
	if (GetLogonAddress() == nullptr)
	{
		DisplayUserMessage("ACARS", "SYSTEM", "Please send your logon address to access. ex) @address RKRR", true, true, false, false, false);
	}

	if (GetLogonCode() != nullptr && GetLogonAddress() != nullptr)
	{
		std::string msg = std::string("All configuration set. Ready to use! - Logon Address : ") + GetLogonAddress() + " / To change: @address (address)";
		DisplayUserMessage("ACARS", "SYSTEM", msg.c_str(), true, true, false, false, false);
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
	std::string sender(sSenderCallsign);
	std::string receiver(sReceiverCallsign);
	std::string message(sChatMessage);

	if (receiver != "ACARS")
		return;

	// check starts with @hoppie
	if (message.rfind("@hoppie", 0) == 0)
	{
		// extract logon code
		std::string logonCode = message.substr(8); // skip "@hoppie "

		// save to settings
		SaveDataToSettings("LogonCode",
							"Hoppie logon code for ACARS",
							logonCode.c_str());

		// send confirmation message
		DisplayUserMessage("ACARS", "SYSTEM", "Hoppie's logon code saved successfully.", true, true, false, false, false);
		return;
	}

	// check starts with @address
	if (message.rfind("@address", 0) == 0)
	{
		// extract logon code
		std::string logonAddress = message.substr(9); // skip "@address "

		// save to settings
		SaveDataToSettings("LogonAddress",
							"Hoppie logon address for ACARS",
							logonAddress.c_str());

		// send confirmation message
		DisplayUserMessage("ACARS", "SYSTEM", "Logon address saved successfully.", true, true, false, false, false);
		return;
	}

	// check starts with @last
	if (message.rfind("@last", 0) == 0)
	{
		// extract logon code
		LastSender = message.substr(6); // skip "@last "

		// send confirmation message
		DisplayUserMessage("ACARS", "SYSTEM", "last sender changed.", true, true, false, false, false);
		return;
	}

	if (LastSender == "")
	{
		DisplayUserMessage("ACARS", "SYSTEM", "no last sender.", true, true, false, false, false);
		return;
	}

	// other case, reply for cpdlc
	// format: /data2/2//NE/FSM 1317 251119 EDDM OCN91D RCD RECEIVED @REQUEST BEING PROCESSED @STANDBY
	std::string url = std::string("http://www.hoppie.nl/acars/system/connect.html?logon=") + GetLogonCode() +
						"&from=" + GetLogonAddress() + "&to=" + LastSender +
						"&type=cpdlc&packet=%2Fdata%2F16%2F" + LastMessageId + "%2FNE%2F" + ConvertCpdlcHttpEncode(message);
	// DisplayUserMessage("ACARS", "DEBUG", url.c_str(), true, true, false, false, false);
	HttpGet(url);
	std::string ackMessage = std::string("CPDLC message sent to ") + LastSender;
	DisplayUserMessage("ACARS", "SYSTEM", ackMessage.c_str(), true, true, false, false, false);
}

const char *CEuroscopeACARSHandler::GetLogonCode()
{
	const char *LogonCode = GetDataFromSettings("LogonCode");
	if (LogonCode == nullptr)
	{
		return nullptr;
	}

	return LogonCode;
}

const char *CEuroscopeACARSHandler::GetLogonAddress()
{
	const char *LogonAddress = GetDataFromSettings("LogonAddress");
	if (LogonAddress == nullptr)
	{
		return nullptr;
	}

	return LogonAddress;
}

void CEuroscopeACARSHandler::OnTimer(int Counter)
{
	try
	{
		if (Counter % 30 == 0)
		{
			const char *LogonCode = GetLogonCode();
			if (LogonCode == nullptr)
				return;

			const char *LogonAddress = GetLogonAddress();
			if (LogonAddress == nullptr)
				return;

			if (!ControllerMyself().IsController())
				return;

			/*
			const char *MyCallsign = ControllerMyself().GetCallsign();
			// check callsign is empty
			if (MyCallsign == nullptr || MyCallsign[0] == '\0')
				return;
			*/

			// prepare url safely
			std::string url = std::string("http://www.hoppie.nl/acars/system/connect.html?logon=") + LogonCode + "&from=" + LogonAddress + "&to=" + LogonAddress + "&type=poll&packet=";
			std::string acars = HttpGet(url);

			// check if acars is starting with "error"
			if (acars.rfind("error", 0) == 0)
			{
				DisplayUserMessage("ACARS", "SYSTEM", acars.c_str(), true, true, false, false, false);
				return;
			}

			// check if acars is just 'ok' string
			if (acars == "ok")
				return;

			// message format sample: ok {TEST cpdlc {/data2/0184/0185/Y/AT @BARKO@ DESCEND TO AND MAINTAIN @FL330@}}
			if (acars.rfind("ok ", 0) != 0)
			{
				DisplayUserMessage("ACARS", "SYSTEM", acars.c_str(), true, true, false, false, false);
				return;
			}

			try
			{

				// remove leading "ok "
				std::string message = acars.substr(4);
				std::string sender = message.substr(0, message.find(' '));
				message = message.substr(message.find(' ') + 1);
				std::string acarstype = message.substr(0, message.find(' '));
				message = message.substr(message.find(' ') + 2);
				// remove last '}} '
				if (message.size() >= 3 && message.substr(message.size() - 3) == "}} ")
				{
					message = message.substr(0, message.size() - 3);
				}

				LastSender = sender;

				// check cpdlc
				if (acarstype == "cpdlc")
				{
					std::string cpdlc = message.substr(message.find('/') + 1);
					std::string messageid = cpdlc.substr(0, message.find('/'));
					cpdlc = cpdlc.substr(message.find('/') + 1);
					std::string replyid = cpdlc.substr(0, message.find('/'));
					cpdlc = cpdlc.substr(message.find('/') + 1);
					std::string ratype = cpdlc.substr(0, message.find('/'));
					cpdlc = cpdlc.substr(message.find('/') + 1);

					LastMessageId = messageid;

					DisplayUserMessage("ACARS", sender.c_str(), cpdlc.c_str(), true, true, true, true, true);
				}
				// normal telex
				else
				{
					LastMessageId = "";

					DisplayUserMessage("ACARS", sender.c_str(), message.c_str(), true, true, true, true, true);
				}
			}
			catch (const std::exception &ee)
			{
				DisplayUserMessage("ACARS", "SYSTEM", ee.what(), true, true, false, false, false);
			}
		}
	}
	catch (const std::exception &e)
	{
		// display error message
		DisplayUserMessage("ACARS", "SYSTEM", e.what(), true, true, false, false, false);
	}
}

void CEuroscopeACARSHandler::OnFunctionCall(int FunctionId, const char *sItemString, POINT Pt, RECT Area)
{
}