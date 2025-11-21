string HttpGet(const string &url)
{
    try
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
    catch (...)
    {
        return "";
    }
}

std::string uppercase(std::string s)
{
    std::ranges::transform(s, s.begin(),
                           [](unsigned char c)
                           { return std::toupper(c); });
    return s;
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