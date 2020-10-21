#include "StdAfx.h"

#include "CefBrowserDlg.h"
#include "afxdialogex.h"


// Диалоговое окно CCefBrowserDlg

IMPLEMENT_DYNAMIC(CCefBrowserDlg, CDialog)

CCefBrowserDlg::CCefBrowserDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_CEF_BROWSER, pParent)
{

}

CCefBrowserDlg::~CCefBrowserDlg()
{
}

void CCefBrowserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CCefBrowserDlg, CDialog)
END_MESSAGE_MAP()


// Обработчики сообщений CCefBrowserDlg
