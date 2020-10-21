#if !defined(AFX_SIGNALOUTPUT_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
#define AFX_SIGNALOUTPUT_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SignalOutputDlg.h : header file
//

#include "SerialPort.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CSignalOutputDlg dialog

class CSignalOutputDlg : public CDialog
{
	// Construction
public:
	CSignalOutputDlg(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CSignalOutputDlg)
	enum { IDD = IDD_SIGNALOUTPUT };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSignalOutputDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow,
		UINT nStatus
		);
	//}}AFX_VIRTUAL

	// Implementation
	void UpdateStatus();

	// Generated message map functions
	//{{AFX_MSG(CSignalOutputDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:

	CUIntArray m_comPortNumbers;
	CListBox m_lbComPorts;
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonSendNow();
	afx_msg void OnEnKillfocusEditTriggerString();
	afx_msg void OnBnClickedRadioBreakSet();
	afx_msg void OnBnClickedRadioBreakClear();
	afx_msg void OnBnClickedRadioDtrSet();
	afx_msg void OnBnClickedRadioDtrClear();
	afx_msg void OnBnClickedRadioRtsSet();
	afx_msg void OnBnClickedRadioRtsClear();
	UINT m_uDTROutputDuration;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIGNALOUTPUT_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
