#if !defined(AFX_SCREENSELECTDLG_H__DD5CA045_E21D_4934_AE6B_CE9DAB18E599__INCLUDED_)
#define AFX_SCREENSELECTDLG_H__DD5CA045_E21D_4934_AE6B_CE9DAB18E599__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ScreenSelectionDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CScreenSelectDlg dialog

class CScreenSelectDlg : public CDialog
{
// Construction
public:
	CScreenSelectDlg(CWnd* pParent = NULL);   // standard constructor
	
// Dialog Data
	//{{AFX_DATA(CScreenSelectDlg)
	enum { IDD = IDD_SCREEN_SELECT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScreenSelectDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, 
							   UINT nStatus  
							);
	afx_msg void OnButtonClicked(UINT nID);
	//}}AFX_VIRTUAL
	void UpdateScreenSelect(int screenID);

	int m_nCurrentScreen;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScreenSelectDlg)
		// NOTE: the ClassWizard will add member functions here
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	void SetScreenSelect(int screen);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCREENSELECTDLG_H__DD5CA045_E21D_4934_AE6B_CE9DAB18E599__INCLUDED_)
