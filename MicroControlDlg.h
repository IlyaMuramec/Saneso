#if !defined(AFX_MICROCONTROLDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
#define AFX_MICROCONTROLDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MicroControlDlg.h : header file
//

#include "SerialPort.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CMicroControlDlg dialog

class CMicroControlDlg : public CDialog
{
	// Construction
public:
	CMicroControlDlg(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CMicroControlDlg)
	enum { IDD = IDD_MICROCONTROL };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMicroControlDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow,
		UINT nStatus
		);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMicroControlDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ReadEEPromToFile(EEPromLocation location);
	void WriteEEPromFromFile(EEPromLocation location);
public:

	CString m_sVersion;
	CString m_sStatus;
	CUIntArray m_comPortNumbers;
	LRESULT OnMicroControllerMessage(WPARAM control, LPARAM value);
	CListBox m_lbComPorts;
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonEepromBoxRead();
	afx_msg void OnBnClickedButtonEepromBoxWrite();
	afx_msg void OnBnClickedButtonEepromScopeRead();
	afx_msg void OnBnClickedButtonEepromScopeWrite();
	CEdit m_editEEPromBox;
	CEdit m_editEEPromScope;
	afx_msg void OnBnClickedButtonEepromBoxReadFile();
	afx_msg void OnBnClickedButtonEepromBoxWriteFile();
	afx_msg void OnBnClickedButtonEepromScopeReadFile();
	afx_msg void OnBnClickedButtonEepromScopeWriteFile();
	afx_msg void OnBnClickedButtonDemoFeatures();
	afx_msg void OnEnKillfocusEditLedMuxPeriod();
	UINT m_uLEDMUXPeriod;
	afx_msg void OnBnClickedButtonReflash();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MICROCONTROLDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
