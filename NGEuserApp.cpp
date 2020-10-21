// NGEuserApp.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "NGEuserDlg.h"
#include "SplashScreen.h"
#include "PasswordDlg.h"
#include "Version.h"
#include <log4cxx\xml\domconfigurator.h>
#include <sstream>

log4cxx::LoggerPtr  CNGEuserApp::logger(log4cxx::Logger::getLogger("NGEuserApp"));

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CNGEuserApp

BEGIN_MESSAGE_MAP(CNGEuserApp, CWinApp)
	//{{AFX_MSG_MAP(NGEuserApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNGEuserApp construction

CNGEuserApp::CNGEuserApp()
{
	m_pMainDlg = NULL;
	m_bIsLoggedIn = false;

	m_bCEFInitialized = FALSE;
	m_bIsClosing = FALSE;
}

CNGEuserApp::~CNGEuserApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNGEuserApp object

CNGEuserApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CNGEuserApp initialization

BOOL CNGEuserApp::InitInstance()
{
	if (InitCef() >= 0)
		return TRUE;

	//Configure logger
	log4cxx::xml::DOMConfigurator::configure("Log4cxxNGEuserConfig.xml");
	logger->getRootLogger()->setLevel(log4cxx::Level::getInfo());

	CStringA version = STRFILEVER;
	version.Replace(", ", ".");

	LOG4CXX_INFO(logger, "Initializing Application v" << version);

	// Make sure that SW isn't already running
	HWND hWnd = ::FindWindow(NULL, _T(WINDOW_NAME));
	if (hWnd != NULL)
	{
		MessageBox(NULL, _T("Saneso software is already running"), _T("Error"), MB_OK | MB_ICONSTOP | MB_TOPMOST);
		return FALSE;
	}

	// Create the Main Dialog
	m_pMainDlg = new CNGEuserDlg();
	m_pMainWnd = m_pMainDlg;

	//ADDITION OF SPLASH SCREEN COMPONENT
	//Bring up the splash screen in a secondary UI thread
	m_pSplashThread = (CSplashThread*)AfxBeginThread(RUNTIME_CLASS(CSplashThread), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	ASSERT(m_pSplashThread->IsKindOf(RUNTIME_CLASS(CSplashThread)));
	if (m_pSplashThread == NULL)
	{
		MessageBox(NULL, _T("Failed to create splash screen"), _T("Error"), MB_OK | MB_ICONSTOP | MB_TOPMOST);
		return FALSE;
	}
	
	// Verify that config folder exists
	CString configFolder = _T(DEFAULT_CONFIG);
	if (GetFileAttributes(configFolder) == INVALID_FILE_ATTRIBUTES) {
		ShowMessageBox("Default config folder not found:\n\n" + configFolder, _T("Error"), MB_OK | MB_TOPMOST);
	}

	try
	{
		m_pSplashThread->SetBitmapToUse(IDB_SANESO_SPLASH);

		m_pSplashThread->ResumeThread();  //Resume the thread now that we have set it up 

		m_SystemManager.Create(m_pMainDlg);


		const INT_PTR nResponse = m_pMainDlg->DoModal();
		if (nResponse == IDOK)
		{
			// TODO: Place code here to handle when the dialog is
			//  dismissed with OK
		}
		else if (nResponse == IDCANCEL)
		{
			// TODO: Place code here to handle when the dialog is
			//  dismissed with Cancel
		}
	}
	catch (std::exception e)
	{
		LOG4CXX_ERROR(logger, e.what());
	}
	catch (...)
	{
		LOG4CXX_ERROR(logger, "Unhandled  exception");
	}

	LOG4CXX_INFO (logger, "Exiting Application");

	m_pSplashThread->DestroySplash();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

bool CNGEuserApp::EditSettings()
{
	CPasswordDlg passwordDlg;
	if (IDOK == passwordDlg.DoModal())
	{
		if (m_pMainDlg)
		{
			m_bIsLoggedIn = true;
			m_pMainDlg->AddTabs(m_bIsLoggedIn);
		}
	}

	return m_bIsLoggedIn;
}

void CNGEuserApp::LogOut()
{
	m_bIsLoggedIn = false;
	m_pMainDlg->AddTabs(m_bIsLoggedIn);
}

CLightingDlg *CNGEuserApp::GetLightingDlg()
{
	CLightingDlg *pDlg = NULL;

	if (m_pMainDlg)
	{
		pDlg = &(m_pMainDlg->m_dLightingDlg);
	}

	return pDlg;
}

CMainDlg *CNGEuserApp::GetOperatorDlg()
{
	CMainDlg *pDlg = NULL;

	if (m_pMainDlg)
	{
		pDlg = &(m_pMainDlg->m_dMainDlg);
	}

	return pDlg;
}

CEnhancementDlg *CNGEuserApp::GetEnhancementDlg()
{
	CEnhancementDlg *pDlg = NULL;

	if (m_pMainDlg)
	{
		pDlg = &(m_pMainDlg->m_dEnhancementDlg);
	}

	return pDlg;

}

CString CNGEuserApp::GetDesktopFolder()
{
	static TCHAR path[MAX_PATH + 1];
	if (SHGetSpecialFolderPath(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE))
		return path;
	else
		return DESKTOP_ERROR_DIR;
}

CString CNGEuserApp::GetImageCaptureFolder()
{
	return CString(GetDesktopFolder() + "\\Saneso Images");
}

CString CNGEuserApp::GetVideoCaptureFolder()
{
	return CString(GetDesktopFolder() + "\\Saneso Videos");
}

int CNGEuserApp::ShowMessageBox(CString text, CString caption, UINT uType)
{
	return m_pSplashThread->ShowMessageBox(CStringA(text), CStringA(caption), uType);
}

int CNGEuserApp::InitCef()
{
	// initialize CEF.
	m_cefApp = new CefClientApp();

	// get arguments
	CefMainArgs main_args(GetModuleHandle(NULL));

	// Execute the secondary process, if any.
	int ret_code = CefExecuteProcess(main_args, m_cefApp.get(), NULL);
	if (ret_code >= 0)
		return ret_code;

	// set settings
	CefSettings settings;
	settings.multi_threaded_message_loop = FALSE;

	void* sandbox_info = NULL;
#if CEF_ENABLE_SANDBOX
	// Manage the life span of the sandbox information object. This is necessary
	// for sandbox support on Windows. See cef_sandbox_win.h for complete details.
	CefScopedSandboxInfo scoped_sandbox;
	sandbox_info = scoped_sandbox.sandbox_info();
#else
	settings.no_sandbox = TRUE;
#endif

	//CEF Initiaized
	m_bCEFInitialized = CefInitialize(main_args, settings, m_cefApp.get(), sandbox_info);

	return ret_code;
}

int CNGEuserApp::ExitInstance()
{
	// shutdown CEF
	if (m_bCEFInitialized) {
		m_cefApp->m_cefHandler = NULL;
		// closing stop work loop
		m_bCEFInitialized = FALSE;
		// release CEF app
		m_cefApp = NULL;
		// clean up CEF loop
		CefDoMessageLoopWork();
		// shutdown CEF
		CefShutdown();
	}

	return CWinApp::ExitInstance();
}

BOOL CNGEuserApp::CreateCefBrowser(HWND hWnd, CRect rect, LPCTSTR pszURL)
{
	return m_cefApp->CreateBrowser(hWnd, rect, pszURL);
}

BOOL CNGEuserApp::PumpMessage()
{
	// do CEF message loop
	if (m_bCEFInitialized && !m_bIsClosing)
	{
		CefDoMessageLoopWork();
	}

	return CWinApp::PumpMessage();
}

void CNGEuserApp::CloseCefBrowser()
{	
	m_cefApp->m_cefHandler->CloseBrowser(false);
//	CefDoMessageLoopWork();
	m_bIsClosing = TRUE;
}

void CNGEuserApp::SendCalibrationStatus(CalStatuses cal_statuses)
{
	m_cefApp->m_cefHandler->SendCalibrationStatus(cal_statuses);
}