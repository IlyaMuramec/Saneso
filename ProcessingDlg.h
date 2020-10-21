#if !defined(AFX_PROCESSINGDLG_H__DD5CA045_E21D_4934_AE6B_CE9DAB18E599__INCLUDED_)
#define AFX_PROCESSINGDLG_H__DD5CA045_E21D_4934_AE6B_CE9DAB18E599__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProcessingDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProcessingDlg dialog

class CProcessingDlg : public CDialog
{
// Construction
public:
	CProcessingDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CProcessingDlg)
	enum { IDD = IDD_PROCESSING };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProcessingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, 
							   UINT nStatus  
							);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void CheckWriteToScopeEnable();

	// Generated message map functions
	//{{AFX_MSG(CProcessingDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonSave();
	afx_msg void OnBnClickedButtonLoad();

	int m_nMosaicWidth;
	int m_nMosaicHeight;
	BOOL m_bBlendingEnabled;
	afx_msg void OnBnClickedCheckBlendImages();
	afx_msg void OnEnKillfocusEditMosaicWidth();
	afx_msg void OnEnKillfocusEditMosaicHeight();
	afx_msg void OnBnClickedButtonPlayback();
	afx_msg void OnBnClickedButtonDoCapture();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnBnClickedButtonLoadCapture();
	afx_msg void OnBnClickedButtonLiveStream();
	afx_msg void OnBnClickedButtonWriteToScope();

	LRESULT OnUpdateModeMessage(WPARAM control, LPARAM value);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROCESSINGDLG_H__DD5CA045_E21D_4934_AE6B_CE9DAB18E599__INCLUDED_)
