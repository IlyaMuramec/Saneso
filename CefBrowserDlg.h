#pragma once

#include "resource.h"

// Диалоговое окно CCefBrowserDlg

class CCefBrowserDlg : public CDialog
{
	DECLARE_DYNAMIC(CCefBrowserDlg)

public:
	CCefBrowserDlg(CWnd* pParent = nullptr);   // стандартный конструктор
	virtual ~CCefBrowserDlg();

// Данные диалогового окна
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CEF_BROWSER };
//#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // поддержка DDX/DDV

	DECLARE_MESSAGE_MAP()
};
