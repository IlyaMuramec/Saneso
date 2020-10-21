#include "afxwin.h"
#include "afxcmn.h"
#if !defined(AFX_SystemManagerDlg_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
#define AFX_SystemManagerDlg_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_

#include "LedButton.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemManagerDlg.h : header file
//

static UINT UWM_UPDATE_SYSTEM_STATUS_MSG = RegisterWindowMessage(_T("Update System Status"));
static UINT UWM__MSG_RESET_USB_CHIPS = RegisterWindowMessage(_T("Reset USB Chips"));

/////////////////////////////////////////////////////////////////////////////
// CSystemManagerDlg dialog

class CSystemManager;

class CSystemManagerDlg : public CDialog
{
	// Construction
public:
	CSystemManagerDlg(CWnd* pParent = NULL);   // standard constructor
	~CSystemManagerDlg();

	BOOL OnInitDialog();

	// Dialog Data
	//{{AFX_DATA(CSystemManagerDlg)
	enum { IDD = IDD_SYSTEM_MANAGER };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSystemManagerDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSystemManagerDlg)
	// NOTE: the ClassWizard will add member functions here
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg LRESULT OnUpdateSystemStatusMessage(WPARAM p1, LPARAM p2);
	afx_msg LRESULT OnResetUSBChipsMessage(WPARAM p1, LPARAM p2);

private:
	CSystemManager *m_pSystemManager;
	CLedButton m_ledMosaic;
	CLedButton m_ledFrontCam;
	CLedButton m_ledTopCam;
	CLedButton m_ledBottomCam;
	CLedButton m_ledLeftCam;
	CLedButton m_ledRightCam;
	CLedButton m_ledController;

	CString m_sVersion;

	void SetupLEDControl(CLedButton *led);
	unsigned int GetCameraSelectMask();
public:
	afx_msg void OnBnClickedButtonReset();
	afx_msg void OnBnClickedSelectAllCam();
	afx_msg void OnBnClickedButtonResetMicroController();
	CString m_sStatus;
	afx_msg void OnBnClickedButtonInitialize();
	afx_msg void OnBnClickedButtonSaveDebugImages();
	afx_msg void OnBnClickedButtonResetUsbChips();
	afx_msg void OnBnClickedCheckLogDebug();
	afx_msg void OnBnClickedButtonCalWhite();
	afx_msg void OnBnClickedButtonCalFPN();
	CString m_sSWVersion;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SystemManagerDlg_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
