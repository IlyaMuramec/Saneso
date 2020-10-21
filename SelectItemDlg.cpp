// SelectItemDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SelectItemDlg.h"
#include "afxdialogex.h"


// CSelectItemDlg dialog

IMPLEMENT_DYNAMIC(CSelectItemDlg, CDialog)

CSelectItemDlg::CSelectItemDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSelectItemDlg::IDD, pParent)
{
	m_sSearchString = _T("*");
	m_bDisplayFolders = FALSE;
}

CSelectItemDlg::~CSelectItemDlg()
{
}

void CSelectItemDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ITEM_LIST, m_lbItemList);
	DDX_Control(pDX, IDC_ITEM_EDIT, m_ebItemEdit);
}


BEGIN_MESSAGE_MAP(CSelectItemDlg, CDialog)
	ON_LBN_SELCHANGE(IDC_ITEM_LIST, &CSelectItemDlg::OnLbnSelchangeItemList)
	ON_LBN_DBLCLK(IDC_ITEM_LIST, &CSelectItemDlg::OnLbnDblclkItemList)
END_MESSAGE_MAP()


// CSelectItemDlg message handlers


BOOL CSelectItemDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(m_sWindowTitle);
	
	WIN32_FIND_DATA ffd;
	CString szDir = m_sSearchFolder + _T("\\") + m_sSearchString;
	HANDLE hFind = FindFirstFile(szDir, &ffd);
	hFind = FindFirstFile(szDir, &ffd);
	if (INVALID_HANDLE_VALUE != hFind)
	{
		do
		{
			if (!m_bDisplayFolders || 
				((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (ffd.cFileName[0] != '.')))
			{
				m_lbItemList.AddString(ffd.cFileName);
			}
		} while (FindNextFile(hFind, &ffd) != 0);
	}

	m_ebItemEdit.EnableWindow(!m_bIsLoading);
	if (!m_bIsLoading)
	{
		m_ebItemEdit.SetSel(0, -1);
		m_ebItemEdit.SetFocus();
	}

	return FALSE;  // return TRUE unless you set the focus to a control
}


void CSelectItemDlg::OnOK()
{
	if (m_ebItemEdit.GetWindowTextLength() > 0)
	{
		m_ebItemEdit.GetWindowText(m_sSelectedItem);

		CDialog::OnOK();
	}
	else
	{
		if (m_bIsLoading)
			ModalMessageDisplay::ShowMessageBox(_T("Please select an item before clicking OK."), _T(""), MB_OK | MB_TOPMOST);
		else
			ModalMessageDisplay::ShowMessageBox(_T("Please type a new name or select an item before clicking OK."), _T(""), MB_OK | MB_TOPMOST);
	}
}


void CSelectItemDlg::OnLbnSelchangeItemList()
{
	CString sSelection;
	m_lbItemList.GetText(m_lbItemList.GetCurSel(), sSelection);
	m_ebItemEdit.SetWindowText(sSelection);

	UpdateData(FALSE);
}


void CSelectItemDlg::OnLbnDblclkItemList()
{
	m_lbItemList.GetText(m_lbItemList.GetCurSel(), m_sSelectedItem);

	CDialog::OnOK();
}
