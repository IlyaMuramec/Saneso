#include "afxwin.h"
#include "afxcmn.h"
#if !defined(AFX_ENHANCEMENTDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
#define AFX_ENHANCEMENTDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EnhancementDlg.h : header file
//

#include "CustomSliderCtrl.h"

class CLightControl;

enum EnhancementHandsetAction
{
	eOnOff,
	eNext,
	ePrevious
};

/////////////////////////////////////////////////////////////////////////////
// CEnhancementDlg dialog


class CEnhancementDlg : public CDialog
{
	// Construction
public:
	CEnhancementDlg(CWnd* pParent = NULL);   // standard constructor

	BOOL OnInitDialog();

	// Dialog Data
	//{{AFX_DATA(CEnhancementDlg)
	enum { IDD = IDD_ENHANCEMENT };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnhancementDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEnhancementDlg)
	// NOTE: the ClassWizard will add member functions here
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ShowPresetButtons();
	void ClearPresetSelect();
	void UpdatePresetSelect();
	void EnableOptions(BOOL enable);
	bool IsButtonSelected(int nIDCtl);

	CEnhancementControl*	m_pEnhancementControl;
	bool m_bNextUpdateFromMC;
	bool m_bNextUpdateFromSlider;

public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedEnchancePreset1();
	afx_msg void OnBnClickedEnchancePreset2();
	afx_msg void OnBnClickedEnchancePreset3();
	afx_msg void OnBnClickedEnchancePreset4();
	afx_msg void OnBnClickedEnchancePreset5();
	afx_msg void OnBnClickedEnchancePreset6();
	afx_msg void OnBnClickedEnchancePreset7();
	afx_msg void OnBnClickedEnchancePreset8();
	CCustomSliderCtrl m_sldrContrast;
	CCustomSliderCtrl m_sldrSharpness;
	CCustomSliderCtrl m_sldrGamma;
	CCustomSliderCtrl m_sldrGammaSide;
	CCustomSliderCtrl m_sldrBrightness;
	CCustomSliderCtrl m_sldrTemporalAveraging;
	CCustomSliderCtrl m_sldrSpatialAveraging;
	CCustomSliderCtrl m_sldrFrontScaleSlope;
	CCustomSliderCtrl m_sldrScaleSlope;
	CCustomSliderCtrl m_sldrScaleOffset;
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedEnhanceContrastOnOff();
	afx_msg void OnBnClickedEnhanceSharpnessOnOff();
	afx_msg void OnBnClickedTemporalAveragingOnOff();
	afx_msg void OnBnClickedButtonTemporalAveragingType();
	afx_msg void OnBnClickedSpatialAveragingOnOff();
	afx_msg void OnBnClickedButtonSpatialAveragingType();
	afx_msg void OnBnClickedEnhanceGammaOnOff();
	afx_msg void OnBnClickedEnhanceBrightnessOnOff();
	afx_msg void OnBnClickedEnhanceGammaSideOnOff();
	afx_msg void OnBnClickedScalingOnOff();
	afx_msg void OnBnClickedEnhanceAllOff();

	//Handset messages
	void HandsetAction(EnhancementHandsetAction action);
	afx_msg void OnBnClickedFrontScalingOnOff();
	afx_msg void OnBnClickedRadioUpsample1();
	afx_msg void OnBnClickedRadioUpsample2();
	afx_msg void OnBnClickedRadioUpsample3();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENHANCEMENTDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
