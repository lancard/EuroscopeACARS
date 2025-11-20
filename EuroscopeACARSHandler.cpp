#include "EuroscopeACARSHandler.h"

#define RGB_YELLOW RGB(255, 255, 0)

string HttpGet(const string &url)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
	wstring wurl(wlen, 0);
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

	string response;
	DWORD sizeAvailable = 0;

	do
	{
		sizeAvailable = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &sizeAvailable))
			break;
		if (sizeAvailable == 0)
			break;

		string buffer(sizeAvailable, 0);
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

string ConvertCpdlcHttpEncode(const string &value)
{
	ostringstream escaped;

	for (char c : value)
	{
		if (c == ' ')
		{
			escaped << '+';
			continue;
		}

		if (c >= 'a' && c <= 'z')
			c -= 32; // 'a'->'A'

		if (isalnum(static_cast<unsigned char>(c)) ||
			c == '-' || c == '_' || c == '.')
		{
			escaped << c;
		}

		if (c == '?' || c == ':' || c == '(' || c == ')' || c == ',' || c == '\'' || c == '=' || c == '/' || c == '+')
		{
			escaped << format("%{:02X}", c);
		}
	}
	return escaped.str();
}

vector<string> split(const string &str, const string &delim)
{
	vector<string> result;
	size_t start = 0, pos;
	while ((pos = str.find(delim, start)) != string::npos)
	{
		result.push_back(str.substr(start, pos - start));
		start = pos + delim.size();
	}
	result.push_back(str.substr(start));
	return result;
}

string trim(const string &s)
{
	size_t start = 0;
	while (start < s.size() && isspace(static_cast<unsigned char>(s[start])))
	{
		++start;
	}

	if (start == s.size())
		return "";

	size_t end = s.size() - 1;
	while (end > start && isspace(static_cast<unsigned char>(s[end])))
	{
		--end;
	}

	return s.substr(start, end - start + 1);
}
CEuroscopeACARSHandler::CEuroscopeACARSHandler(void) : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE,
															   "EuroscopeACARS",
															   "0.9",
															   "Sung-ho Kim",
															   "Sung-ho Kim")
{
	DisplayUserMessage("Message", "EuroscopeACARS", string("ACARS Loaded.").c_str(), false, false, false, false, false);

	if (GetLogonCode() == nullptr)
	{
		DisplayUserMessage("ACARS", "SYSTEM", "Please send your hoppie's logon code to access. ex) .hoppie (logoncode)", true, true, false, false, false);
	}
	if (GetLogonAddress() == nullptr)
	{
		DisplayUserMessage("ACARS", "SYSTEM", "Please send your logon address to access. ex) .address RKRR", true, true, false, false, false);
	}

	if (GetLogonCode() != nullptr && GetLogonAddress() != nullptr)
	{
		string msg = format(
			"All configuration set. Ready to use! (Logon Address : {}) / To change: .address (address)",
			GetLogonAddress());
		DisplayUserMessage("ACARS", "SYSTEM", msg.c_str(), true, true, false, false, false);
	}
}

CEuroscopeACARSHandler::~CEuroscopeACARSHandler(void)
{
	DisplayUserMessage("Message", "EuroscopeACARS", "ACARS Unloaded.", false, false, false, false, false);
}

void CEuroscopeACARSHandler::DebugPrint(string message)
{
	if (!DebugMode)
		return;

	DisplayUserMessage("ACARS", "DEBUG", message.c_str(), true, true, false, false, false);
}

bool CEuroscopeACARSHandler::OnCompileCommand(const char *sCommandLine)
{
	string message(sCommandLine);

	// check starts with .hoppie
	if (message.rfind(".hoppie ", 0) == 0)
	{
		// extract logon code
		string logonCode = message.substr(8); // skip ".hoppie "

		// save to settings
		SaveDataToSettings("LogonCode",
						   "Hoppie logon code for ACARS",
						   logonCode.c_str());

		// send confirmation message
		DisplayUserMessage("ACARS", "SYSTEM", "Hoppie's logon code saved successfully.", true, true, false, false, false);
		return true;
	}

	// check starts with .address
	if (message.rfind(".address ", 0) == 0)
	{
		// extract logon code
		string logonAddress = message.substr(9); // skip ".address "

		// save to settings
		SaveDataToSettings("LogonAddress",
						   "Hoppie logon address for ACARS",
						   logonAddress.c_str());

		// send confirmation message
		DisplayUserMessage("ACARS", "SYSTEM", "Logon address saved successfully.", true, true, false, false, false);
		return true;
	}

	// check starts with .address
	if (message.rfind(".acarsdebug", 0) == 0)
	{
		DebugMode = true;
		return true;
	}

	return false;
}

void CEuroscopeACARSHandler::OnCompilePrivateChat(const char *sSenderCallsign,
												  const char *sReceiverCallsign,
												  const char *sChatMessage)
{
	string sender(sSenderCallsign);
	string receiver(sReceiverCallsign);
	string message(sChatMessage);

	if (receiver.rfind("ACARS-", 0) != 0)
		return;

	// Get Callsign from receiver
	string callsign = receiver.substr(6);

	string LastMessageId = "";
	if (LastMessageIdMap.contains(callsign))
		LastMessageId = LastMessageIdMap[callsign];

	// other case, reply for cpdlc
	// format: /data2/2//NE/FSM 1317 251119 EDDM OCN91D RCD RECEIVED @REQUEST BEING PROCESSED @STANDBY
	// NE: no reply required
	// WU: wilco / unable
	string url = format(
		"http://www.hoppie.nl/acars/system/connect.html?logon={}&from={}&to={}&type=cpdlc&packet=%2Fdata%2F16%2F{}%2FWU%2F{}",
		GetLogonCode(),
		GetLogonAddress(),
		callsign,
		LastMessageId,
		ConvertCpdlcHttpEncode(message));
	DebugPrint(url);
	HttpGet(url);
	string ackMessage = format("CPDLC message sent to {}", callsign);
	DisplayUserMessage(receiver.c_str(), "SYSTEM", ackMessage.c_str(), true, true, false, false, false);
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

void CEuroscopeACARSHandler::ProcessMessage(string message)
{
	// message format sample: TEST cpdlc {/data2/0184/0185/Y/AT @BARKO@ DESCEND TO AND MAINTAIN @FL330@
	try
	{
		string sender = message.substr(0, message.find(' '));
		message = message.substr(message.find(' ') + 1);
		string acarstype = message.substr(0, message.find(' '));
		message = message.substr(message.find(' ') + 2);

		string acarssender = string("ACARS-") + sender;

		// check cpdlc
		if (acarstype == "cpdlc")
		{
			string cpdlc = message.substr(message.find('/') + 1);
			string messageid = cpdlc.substr(0, cpdlc.find('/'));
			cpdlc = cpdlc.substr(cpdlc.find('/') + 1);
			string replyid = cpdlc.substr(0, cpdlc.find('/'));
			cpdlc = cpdlc.substr(cpdlc.find('/') + 1);
			string ratype = cpdlc.substr(0, cpdlc.find('/'));
			cpdlc = cpdlc.substr(cpdlc.find('/') + 1);

			LastMessageIdMap[sender] = messageid;

			DisplayUserMessage(acarssender.c_str(), messageid.c_str(), cpdlc.c_str(), true, true, true, true, true);
		}
		// normal telex
		else
		{
			DisplayUserMessage(acarssender.c_str(), sender.c_str(), message.c_str(), true, true, true, true, true);
		}
	}
	catch (const exception &ee)
	{
		DebugPrint(string(ee.what()));
	}
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
			string url = format(
				"http://www.hoppie.nl/acars/system/connect.html?logon={}&from={}&to={}&type=poll&packet=",
				LogonCode,
				LogonAddress,
				LogonAddress);
			string acars = HttpGet(url);

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

			// remove leading "ok "
			string message = acars.substr(4);

			// split by }}
			vector<string> messages = split(message, "}}");
			for (string &t : messages)
			{
				string k = trim(t).substr(1); // passing '{'
				DebugPrint(k);
				ProcessMessage(k);
			}
		}
	}
	catch (const exception &e)
	{
		// display error message
		DisplayUserMessage("ACARS", "SYSTEM", e.what(), true, true, false, false, false);
	}
}

void CEuroscopeACARSHandler::OnFunctionCall(int FunctionId, const char *sItemString, POINT Pt, RECT Area)
{
}