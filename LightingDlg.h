#include "afxwin.h"
#include "afxcmn.h"
#if !defined(AFX_LIGHTINGDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
#define AFX_LIGHTINGDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LightingDlg.h : header file
//

#include "CustomSliderCtrl.h"

class CLightControl;

/////////////////////////////////////////////////////////////////////////////
// CLightingDlg dialog


class CLightingDlg : public CDialog
{
// Construction
public:
	CLightingDlg(CWnd* pParent = NULL);   // standard constructor
	
	BOOL OnInitDialog();

// Dialog Data
	//{{AFX_DATA(CLightingDlg)
	enum { IDD = IDD_LIGHTING };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLightingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLightingDlg)
		// NOTE: the ClassWizard will add member functions here
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ShowColorButtonSelect();

	void ClearFrontColorSelect();
	void EnableFrontColorSelect(BOOL enable);
	void UpdateFrontColorSelect();

	void ClearSideColorSelect();
	void EnableSideColorSelect(BOOL enable);
	void UpdateSideColorSelect();

	void UpdateFrontLevel();
	void UpdateSideLevel();
	void UpdateAutoControls();

	CLightControl*	m_pLightControl;
	bool m_bNextUpdateFromMC;
	bool m_bNextUpdateFromSlider;

public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
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
	CBitmapButton m_btnLEDFrontDown;
	CBitmapButton m_btnLEDFrontUp;
	afx_msg void OnBnClickedLedFrontOnOff();
	afx_msg void OnBnClickedLedFrontAuto();
	CBitmapButton m_btnLEDSideDown;
	CBitmapButton m_btnLEDSideUp;
	afx_msg void OnBnClickedLedSideOnOff();
	afx_msg void OnBnClickedLedSideAuto();
	CProgressCtrl m_ctlLEDFrontLevel;
	CProgressCtrl m_ctlLEDSideLevel;
	afx_msg void OnBnClickedLedFrontDown();
	afx_msg void OnBnClickedLedFrontUp();
	afx_msg void OnBnClickedLedSideDown();
	afx_msg void OnBnClickedLedSideUp();
	afx_msg void OnBnClickedShadowCastOnOff();
	afx_msg void OnBnClickedCheckShadowFront1();
	afx_msg void OnBnClickedCheckShadowLeft1();
	afx_msg void OnBnClickedCheckShadowRight1();
	afx_msg void OnBnClickedCheckShadowTop1();
	afx_msg void OnBnClickedCheckShadowBottom1();
	afx_msg void OnBnClickedCheckShadowFront2();
	afx_msg void OnBnClickedCheckShadowLeft2();
	afx_msg void OnBnClickedCheckShadowRight2();
	afx_msg void OnBnClickedCheckShadowTop2();
	afx_msg void OnBnClickedCheckShadowBottom2();
	CSliderCtrl m_sldrShadowCastDelay;
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnMicroControllerMessage(WPARAM control, LPARAM value);
	afx_msg LRESULT OnUpdateLightValue(WPARAM control, LPARAM value);
	afx_msg LRESULT OnSelectBandImaging(WPARAM control, LPARAM value);

	//Handset messages
	void CycleColor(bool bNext);
	void ToggleFrontLights();
	void ToggleSideLights();
	CSliderCtrl m_sldrFrontValue;
	CSliderCtrl m_sldrLeftValue;
	CSliderCtrl m_sldrRightValue;
	CSliderCtrl m_sldrTopValue;
	CSliderCtrl m_sldrBottomValue;
	CCustomSliderCtrl m_sldrAutoSetpoint;
	CCustomSliderCtrl m_sldrAutoGainP;
	CCustomSliderCtrl m_sldrAutoGainI;
	CCustomSliderCtrl m_sldrAutoGainD;
	afx_msg void OnBnClickedAutoSatOrAvg();
	afx_msg void OnBnClickedAutoMode();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIGHTINGDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
