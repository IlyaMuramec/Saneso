#include "afxwin.h"
#include "afxcmn.h"
#if !defined(AFX_SelfCalibrationDlg_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
#define AFX_SelfCalibrationDlg_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_

#include "SelfCalibration.h"
#include "LedButton.h"


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelfCalibrationDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelfCalibrationDlg dialog

static UINT UWM_UPDATE_SELF_CALIBRATION_MSG = RegisterWindowMessage(_T("Update Self Cal"));
static UINT UWM_LAST_SELF_CALIBRATION_MSG = RegisterWindowMessage(_T("Last Self Cal"));


class CSelfCalibrationDlg : public CDialog
{
	// Construction
public:
	CSelfCalibrationDlg(CWnd* pParent = NULL);   // standard constructor
	~CSelfCalibrationDlg();

	BOOL OnInitDialog();

	// Dialog Data
	//{{AFX_DATA(CSelfCalibrationDlg)
	enum { IDD = IDD_SELF_CALIBRATION };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	LRESULT OnUpdateSelfCalibrationMessage(WPARAM p1, LPARAM p2);
	LRESULT OnLastSelfCalibrationMessage(WPARAM p1, LPARAM p2);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelfCalibrationDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelfCalibrationDlg)
	// NOTE: the ClassWizard will add member functions here
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void SetupLEDControl(CLedButton *led);

public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

private:
	CSelfCalibration *m_pSelfCalibration;

	CLedButton m_ledLastCal;
	CLedButton m_ledCameras;
	CLedButton m_ledCalCup;
	CLedButton m_ledLights;
	CLedButton m_ledFPN;
	CLedButton m_ledWB;
	CLedButton m_ledOverall;

	CFont m_StartButtonFont;

public:
	afx_msg void OnBnClickedButtonStartSelfCal();
	CString m_sLastCalTime;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SelfCalibrationDlg_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
