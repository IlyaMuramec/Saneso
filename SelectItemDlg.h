#pragma once

#include "resource.h"
#include "afxwin.h"

// CSelectItemDlg dialog

class ModalMessageDisplay
{
public:
	static int ShowMessageBox(CString text, CString caption, UINT uType);
};

class CSelectItemDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelectItemDlg)

public:
	CSelectItemDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelectItemDlg();

// Dialog Data
	enum { IDD = IDD_SELECT_ITEM };

	void SetTitle(CString sTitle) { m_sWindowTitle = sTitle; }
	void DisplayFolders(BOOL val) { m_bDisplayFolders = val; }
	void IsLoading(BOOL val)	{ m_bIsLoading = val; }
	void SetSearchFolder(CString str) { m_sSearchFolder = str; }
	void SetSearchString(CString str) { m_sSearchString = str; }
	CString GetSelectedItem() { return m_sSelectedItem; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	CString m_sWindowTitle;
	BOOL m_bIsLoading;
	BOOL m_bDisplayFolders;
	CString m_sSelectedItem;
	CString m_sSearchFolder;
	CString m_sSearchString;

public:
	CListBox m_lbItemList;
	CEdit m_ebItemEdit;
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnLbnSelchangeItemList();
	afx_msg void OnLbnDblclkItemList();
};
