// PasswordDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PasswordDlg.h"
#include "afxdialogex.h"
#include "NGEuserApp.h"

// CPasswordDlg dialog

IMPLEMENT_DYNAMIC(CPasswordDlg, CDialog)

CPasswordDlg::CPasswordDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPasswordDlg::IDD, pParent)
	, m_Password(_T(""))
{

}

CPasswordDlg::~CPasswordDlg()
{
}

void CPasswordDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, m_EditPassword);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_Password);
}


BEGIN_MESSAGE_MAP(CPasswordDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CPasswordDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CPasswordDlg message handlers

BOOL CPasswordDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_EditPassword.SetSel(0, -1);
	m_EditPassword.SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
}

void CPasswordDlg::OnBnClickedOk()
{
	UpdateData();

	if (_T("Saneso") != m_Password)
	{
		theApp.ShowMessageBox(_T("The password is incorrect"), _T("Error"), MB_OK | MB_TOPMOST);
		m_EditPassword.SetSel(0, -1);
		m_EditPassword.SetFocus();
		return;
	}

	UpdateData(FALSE);
	CDialog::OnOK();
}
