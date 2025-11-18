#include "EuroscopeACARS.h"
#include "EuroscopeACARSHandler.h"

BEGIN_MESSAGE_MAP(CEuroscopeACARSApp, CWinApp)
END_MESSAGE_MAP()

CEuroscopeACARSApp::CEuroscopeACARSApp()
{
}

CEuroscopeACARSApp theApp;

// CEuroscopeACARSApp initialization
BOOL CEuroscopeACARSApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}

CEuroscopeACARSHandler * gpMyPlugin = NULL;

void __declspec (dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn ** ppPlugInInstance)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// create the instance
	*ppPlugInInstance = gpMyPlugin = new CEuroscopeACARSHandler();
}

void __declspec (dllexport) EuroScopePlugInExit(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// delete the instance
	delete gpMyPlugin;
}
