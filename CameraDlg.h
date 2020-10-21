#include "afxwin.h"
#if !defined(AFX_CAMERADLG_H__289C169F_2DD3_4533_ABDB_8E0CC0CA993C__INCLUDED_)
#define AFX_CAMERADLG_H__289C169F_2DD3_4533_ABDB_8E0CC0CA993C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CameraDlg.h : header file
//

#include "CameraData.h"

/////////////////////////////////////////////////////////////////////////////
// CCameraDlg dialog

class CCameraDlg : public CDialog
{
// Construction
public:
	CCameraDlg(CWnd* pParent = NULL);   // standard constructor
	~CCameraDlg();

// Dialog Data
	//{{AFX_DATA(CCameraDlg)
	enum { IDD = IDD_CAMERA };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCameraDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, 
							   UINT nStatus  
							);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	//}}AFX_VIRTUAL


// Implementation
protected:

	CameraData *m_pCurrentCamera;
	cv::Mat m_cvImg;
	bool m_bIsLive;

	void FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin);

	// Generated message map functions
	//{{AFX_MSG(CCameraDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	
	float m_fRedGain;
	float m_fGreenGain;
	float m_fBlueGain;
	CComboBox m_cbCameraSelect;
	afx_msg void OnCbnSelchangeComboCamSelect();

private:
	bool _bExiting;
	HANDLE _hThread;
	HANDLE _hCameraEvent;
	static UINT ThreadProc(LPVOID param)
	{
		return ((CCameraDlg*)param)->CaptureLoop();
	}

	UINT CaptureLoop();
public:
	int m_nBayerType;
	afx_msg void OnEnKillfocusEditBayerType();
	afx_msg void OnEnKillfocusEditRedGain();
	afx_msg void OnEnKillfocusEditGreenGain();
	afx_msg void OnEnKillfocusEditBlueGain();
	afx_msg void OnBnClickedButtonWhiteCal();
	afx_msg void OnBnClickedCheckAutoExposure();
	afx_msg void OnBnClickedCheckAutoGain();
	afx_msg void OnBnClickedCheckTestPattern();
	BOOL m_bTestPatternEnabled;
	BOOL m_bAutoExposureEnabled;
	BOOL m_bAutoGainEnabled;
	afx_msg void OnEnKillfocusEditExposureTime();
	afx_msg void OnEnKillfocusEditGain();
	int m_nExposureSetting;
	int m_nGainSetting;
	UINT    m_uTimerVal;
	afx_msg void OnBnClickedButtonFpnCal();

	void StopCaptureThread();
	double m_dGamma;
	CString m_sRegisterAddress;
	CString m_sRegisterValue;
	afx_msg void OnBnClickedButtonRegisterRead();
	afx_msg void OnBnClickedButtonRegisterWrite();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CAMERADLG_H__289C169F_2DD3_4533_ABDB_8E0CC0CA993C__INCLUDED_)
