#if !defined(AFX_FPGADLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
#define AFX_FPGADLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LightingDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFPGADlg dialog

class CFPGADlg : public CDialog
{
// Construction
public:
	CFPGADlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFPGADlg)
	enum { IDD = IDD_FPGA };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFPGADlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow,
		UINT nStatus
		);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFPGADlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCheckFilterFrames();
	afx_msg void OnBnClickedButtonSaveNextTransfer();

	CString m_sVersion;
	BOOL m_bFilterFrames;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FPGADLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
