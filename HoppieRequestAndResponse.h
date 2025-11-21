
class HoppieRequest
{
public:
    static inline int GlobalMessageId = 1000;
    string LogonCode;
    string From;
    string To;
    string Type;
    string Packet;

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

            if (isalnum(static_cast<unsigned char>(c)))
            {
                escaped << c;
                continue;
            }

            if (c == '-' || c == '.' || c == '@')
            {
                escaped << c;
                continue;
            }
        }

        return escaped.str();
    }

    HoppieRequest(const string &logonCode, const string &from, const string &to, const string &type, const string &packet)
    {
        LogonCode = logonCode;
        From = from;
        To = to;
        Type = type;
        Packet = packet;
    }

    HoppieRequest(const string &logonCode, const string &from, const string &to, const string &replyid, const string &cpdlcratype, const string &cpdlcmessage)
    {
        LogonCode = logonCode;
        From = from;
        To = to;
        Type = "cpdlc";
        Packet = format("%2Fdata2%2F{}%2F{}%2F{}%2F{}",
                        GlobalMessageId,
                        replyid,
                        cpdlcratype,
                        ConvertCpdlcHttpEncode(cpdlcmessage));

        GlobalMessageId += 10;
    }

    string GetUrl()
    {
        string url = format(
            "http://www.hoppie.nl/acars/system/connect.html?logon={}&from={}&to={}&type={}&packet={}",
            LogonCode,
            From,
            To,
            Type,
            Packet);

        return url;
    }
};

class HoppieResponse
{
public:
    HoppieRequest request;
    string response;

    HoppieResponse(HoppieRequest req, string res) : request(move(req)), response(move(res))
    {
    }
};