#include "afxwin.h"
#if !defined(AFX_MAINDLG_H__C840A279_AAA1_48F6_A389_2D5BE1CDBEFE__INCLUDED_)
#define AFX_MAINDLG_H__C840A279_AAA1_48F6_A389_2D5BE1CDBEFE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MainDlg.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CMainDlg dialog

class CMainDlg : public CDialog
{
// Construction
public:
	CMainDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMainDlg)
	enum { IDD = IDD_MAIN };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMainDlg)
		// NOTE: the ClassWizard will add member functions here
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedButtonEditSettings();
	afx_msg void OnBnClickedButtonSelectBandImaging();
	afx_msg LRESULT OnMicroControllerMessage(WPARAM control, LPARAM value);
	afx_msg LRESULT OnSelectBandImaging(WPARAM control, LPARAM value);
	afx_msg void OnBnClickedLedFrontColor1();
	afx_msg void OnBnClickedLedFrontColor2();
	afx_msg void OnBnClickedLedFrontColor3();
	afx_msg void OnBnClickedLedFrontColor4();
	afx_msg void OnBnClickedLedFrontColor5();
	afx_msg void OnBnClickedLedFrontColor6();
	afx_msg void OnBnClickedLedFrontColor7();
	afx_msg void OnBnClickedLedFrontColor8();
	afx_msg void OnBnClickedLedSideColor1();
	afx_msg void OnBnClickedLedSideColor2();
	afx_msg void OnBnClickedLedSideColor3();
	afx_msg void OnBnClickedLedSideColor4();
	afx_msg void OnBnClickedLedSideColor5();
	afx_msg void OnBnClickedLedSideColor6();
	afx_msg void OnBnClickedLedSideColor7();
	afx_msg void OnBnClickedLedSideColor8();
	afx_msg LRESULT OnUpdateModeMessage(WPARAM control, LPARAM value);

	void UpdateLightColor();

private:
	void ShowColorButtonSelect();

	void ClearFrontColorSelect();
	void EnableFrontColorSelect(BOOL enable);
	void UpdateFrontColorSelect();

	void ClearSideColorSelect();
	void EnableSideColorSelect(BOOL enable);
	void UpdateSideColorSelect();

	CMicroController    *m_pMicroController;
	CLightControl       *m_pLightControl;
public:
	afx_msg void OnBnClickedButtonLiveStream();
	afx_msg void OnBnClickedButtonPlayback();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnBnClickedButtonLoadCapture();
	afx_msg void OnBnClickedButtonDoCapture();
	afx_msg void OnBnClickedButtonDefaultSettings();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINDLG_H__C840A279_AAA1_48F6_A389_2D5BE1CDBEFE__INCLUDED_)
