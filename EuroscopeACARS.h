#pragma once
#include <SDKDDKVer.h>
#include <afxwin.h>

class CEuroscopeACARSApp : public CWinApp
{
public:
	CEuroscopeACARSApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
