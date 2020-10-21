// FPGADlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "FPGADlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFPGADlg dialog


CFPGADlg::CFPGADlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFPGADlg::IDD, pParent)
	, m_sVersion(_T(""))
	, m_bFilterFrames(TRUE)
{
	//{{AFX_DATA_INIT(CFPGADlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CFPGADlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFPGADlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT_VERSION, m_sVersion);
	DDX_Check(pDX, IDC_CHECK_FILTER_FRAMES, m_bFilterFrames);
}


BEGIN_MESSAGE_MAP(CFPGADlg, CDialog)
	//{{AFX_MSG_MAP(CFPGADlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_CHECK_FILTER_FRAMES, &CFPGADlg::OnBnClickedCheckFilterFrames)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_NEXT_TRANSFER, &CFPGADlg::OnBnClickedButtonSaveNextTransfer)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFPGADlg message handlers

BOOL CFPGADlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CFPGADlg::OnShowWindow(BOOL bShow,
	UINT nStatus)
{
	if (bShow)
	{
#ifndef USE_USB_CAMERA
		m_sVersion = theApp.GetSystemManager()->GetPentekReader()->GetVersion().c_str();
		m_bFilterFrames = theApp.GetSystemManager()->GetPentekReader()->IsFilterFramesEnabled();
#endif

		UpdateData(FALSE);
	}
	else
	{
	}
}

void CFPGADlg::OnBnClickedCheckFilterFrames()
{
	UpdateData();

#ifndef USE_USB_CAMERA
	theApp.GetSystemManager()->GetPentekReader()->EnableFilterFrames(m_bFilterFrames);
#endif
}

void CFPGADlg::OnBnClickedButtonSaveNextTransfer()
{
#ifndef USE_USB_CAMERA
	theApp.GetSystemManager()->GetPentekReader()->SaveNextTransfer();
#endif
}

