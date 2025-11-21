#include "EuroscopeACARSHandler.h"
#include "EuroscopeUtils.h"

#define RGB_YELLOW RGB(255, 255, 0)

CEuroscopeACARSHandler::CEuroscopeACARSHandler(void) : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE,
															   "EuroscopeACARS",
															   PROGRAM_VERSION,
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

	DebugPrint("Debug Mode on!");

	workerThread = thread(&CEuroscopeACARSHandler::ThreadRunner, this);
}

CEuroscopeACARSHandler::~CEuroscopeACARSHandler(void)
{
	terminateSignal = true;
	if (workerThread.joinable())
		workerThread.join();
	DisplayUserMessage("Message", "EuroscopeACARS", "ACARS Unloaded.", false, false, false, false, false);
}

void CEuroscopeACARSHandler::ThreadRunner()
{
	while (!terminateSignal.load())
	{
		optional<HoppieRequest> req = requestQueue.Dequeue();

		if (req.has_value())
		{
			string res = HttpGet(req.value().GetUrl());

			responseQueue.Enqueue(HoppieResponse(req.value(), res));
		}

		this_thread::sleep_for(chrono::seconds(1));
	}
}

void CEuroscopeACARSHandler::SendToHoppie(HoppieRequest req)
{
	requestQueue.Enqueue(req);
}

void CEuroscopeACARSHandler::DebugPrint(string message)
{
	const char *DebugMode = GetDataFromSettings("AcarsDebugMode");
	if (DebugMode == nullptr || DebugMode[0] != 'o')
	{
		return;
	}

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
		// save to settings
		SaveDataToSettings("AcarsDebugMode",
						   "ACARS Debug Mode",
						   "on");
		DisplayUserMessage("ACARS", "SYSTEM", "Debug mode on.", true, true, false, false, false);
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
	message = uppercase(message);

	if (receiver.rfind("ACARS-", 0) != 0)
		return;

	// Get Callsign from receiver
	vector<string> callsign_and_target = split(receiver.substr(6), "-");
	string callsign = callsign_and_target[0];
	string target = callsign_and_target[1];

	string LastMessageId = "";
	if (LastMessageIdMap.contains(callsign))
		LastMessageId = LastMessageIdMap[callsign];

	// other case, reply for cpdlc
	// format: /data2/2//NE/FSM 1317 251119 EDDM OCN91D RCD RECEIVED @REQUEST BEING PROCESSED @STANDBY
	// NE: no reply required
	// WU: wilco / unable
	if (message == "ROGER" || message == "RGR" || message == "AFFIRM" || message == "AFFIRMATIVE" || message == "NEG" || message == "NEGATIVE")
	{
		SendToHoppie(HoppieRequest(GetLogonCode(), callsign, target, LastMessageId, "NE", message));
	}
	else
	{
		SendToHoppie(HoppieRequest(GetLogonCode(), callsign, target, LastMessageId, "WU", message));
	}
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

void CEuroscopeACARSHandler::ProcessMessage(string callsign, string message)
{
	// message format sample: TEST cpdlc {/data2/0184/0185/Y/AT @BARKO@ DESCEND TO AND MAINTAIN @FL330@
	try
	{
		string sender = message.substr(0, message.find(' '));
		message = message.substr(message.find(' ') + 1);
		string acarstype = message.substr(0, message.find(' '));
		message = message.substr(message.find(' ') + 2);

		string acarssender = string("ACARS-") + callsign + "-" + sender;

		// check cpdlc
		if (acarstype == "cpdlc")
		{
			message = message.substr(message.find('/') + 1); // pass first ' {/'

			string cpdlc = message.substr(message.find('/') + 1);
			string messageid = cpdlc.substr(0, cpdlc.find('/'));
			cpdlc = cpdlc.substr(cpdlc.find('/') + 1);
			string replyid = cpdlc.substr(0, cpdlc.find('/'));
			cpdlc = cpdlc.substr(cpdlc.find('/') + 1);
			string ratype = cpdlc.substr(0, cpdlc.find('/'));
			cpdlc = cpdlc.substr(cpdlc.find('/') + 1);

			LastMessageIdMap[sender] = messageid;

			if (cpdlc == "REQUEST LOGON")
			{
				SendToHoppie(HoppieRequest(GetLogonCode(), callsign, sender, messageid, "NE", "LOGON ACCEPTED"));
				DisplayUserMessage(acarssender.c_str(), sender.c_str(), "Logged On!", true, true, false, false, false);
				return;
			}
			if (cpdlc == "LOGOFF")
			{
				DisplayUserMessage(acarssender.c_str(), sender.c_str(), "Logged Off!", true, true, false, false, false);
				return;
			}

			DisplayUserMessage(acarssender.c_str(), sender.c_str(), cpdlc.c_str(), true, true, false, true, false);
		}
		// normal telex
		else
		{
			DisplayUserMessage(acarssender.c_str(), sender.c_str(), message.c_str(), true, true, false, true, false);
		}
	}
	catch (const exception &ee)
	{
		DebugPrint(string(ee.what()));
	}
}

void CEuroscopeACARSHandler::OnTimerRequestPolling()
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

	string LogonAddressList(LogonAddress);
	vector<string> list = split(LogonAddressList, ",");
	for (const string &callsign : list)
	{
		SendToHoppie(HoppieRequest(GetLogonCode(), callsign, callsign, "poll", ""));
	}
}

void CEuroscopeACARSHandler::OnTimerProcessResponse()
{
	optional<HoppieResponse> res = responseQueue.Dequeue();
	if (!res.has_value())
		return;

	if (res.value().request.Type != "poll")
		return;

	string acars = res.value().response;

	if (acars == "")
		return;

	// process request
	// check if acars is starting with "error"
	if (acars.rfind("error", 0) == 0)
	{
		DisplayUserMessage("ACARS", "SYSTEM", acars.c_str(), true, true, false, false, false);
		return;
	}

	// check if acars is just 'ok' string
	if (acars == "ok")
		return;

	DebugPrint(acars);

	// message format sample: ok {TEST cpdlc {/data2/0184/0185/Y/AT @BARKO@ DESCEND TO AND MAINTAIN @FL330@}}
	if (acars.rfind("ok ", 0) != 0)
	{
		DisplayUserMessage("ACARS", "SYSTEM", acars.c_str(), true, true, false, false, false);
		return;
	}

	// remove leading "ok "
	string message = acars.substr(3);

	// split by }}
	vector<string> messages = split(message, "}}");
	for (string &t : messages)
	{
		if (t.length() < 5)
			continue;
		string k = trim(t).substr(1); // passing '{'
		ProcessMessage(res.value().request.From, k);
	}
}

void CEuroscopeACARSHandler::OnTimer(int Counter)
{
	try
	{
		// polling request
		if (Counter % 30 == 0)
		{
			OnTimerRequestPolling();
		}

		OnTimerProcessResponse();
	}
	catch (const exception &e)
	{
		// display error message
		DebugPrint(e.what());
	}
}

void CEuroscopeACARSHandler::OnFunctionCall(int FunctionId, const char *sItemString, POINT Pt, RECT Area)
{
}