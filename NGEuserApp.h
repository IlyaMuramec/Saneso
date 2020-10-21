// NGEuserApp.h : main header file for the SETTINGS application
//

#if !defined(AFX_NGEuserAPP_H__58CB1501_DDF8_463A_90BB_97459440FA01__INCLUDED_)
#define AFX_NGEuserAPP_H__58CB1501_DDF8_463A_90BB_97459440FA01__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

#include "cef/CefClientApp.h"

#include "afxmt.h"
#include <vector>
#include "SystemManager.h"

#define WM_UPDATE_MODE_MSG WM_APP + 0x100
#define WM_UPDATE_SCREEN WM_APP + 0x101

#define DEFAULT_CONFIG "C:\\NGE\\Config\\Default"
#define DEBUG_FOLDER "C:\\NGE\\DebugImages"
#define DESKTOP_ERROR_DIR "C:\\NGE\\"
#define WINDOW_NAME "NGE"
#define DEBUG_MONITOR_JAVQUI

/////////////////////////////////////////////////////////////////////////////
// CNGEuserApp:
// See CNGEuserApp.cpp for the implementation of this class
//
class CNGEuserDlg;
class CLightingDlg;
class CMainDlg;
class CEnhancementDlg;


class CNGEuserApp : public CWinApp
{
public:
	CNGEuserApp();
	~CNGEuserApp();

	CSystemManager* GetSystemManager()	{ return &m_SystemManager; }
	CNGEuserDlg *GetMainDlg()	{ return m_pMainDlg; }
	CLightingDlg *GetLightingDlg();
	CMainDlg *GetOperatorDlg();
	CEnhancementDlg *GetEnhancementDlg();
	CSplashThread *GetSplashThread()		{ return m_pSplashThread; }

	CString GetDesktopFolder();
	CString GetImageCaptureFolder();
	CString GetVideoCaptureFolder();

	bool IsLoggedIn()				{ return m_bIsLoggedIn; }
	bool EditSettings();
	void LogOut();
	int ShowMessageBox(CString text, CString caption, UINT uType);
		
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNGEuserApp)
	public:
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;
	virtual BOOL PumpMessage() override;
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CNGEuserApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Cef section

public:
	BOOL CreateCefBrowser(HWND hWnd, CRect rect, LPCTSTR pszURL);
	void CloseCefBrowser();

	void SendCalibrationStatus(CalStatuses cal_statuses);

private:
	int  InitCef();

	BOOL m_bCEFInitialized;
	BOOL m_bIsClosing;
	CefRefPtr<CefClientApp> m_cefApp;

private:
	static log4cxx::LoggerPtr logger;

	CSystemManager m_SystemManager;
	CNGEuserDlg	*m_pMainDlg;
	CSplashThread	*m_pSplashThread;

	bool m_bIsLoggedIn;

	friend class MessageHandler;
};

extern CNGEuserApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NGEuserAPP_H__58CB1501_DDF8_463A_90BB_97459440FA01__INCLUDED_)
