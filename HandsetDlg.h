#if !defined(AFX_HANDESETDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)
#define AFX_HANDESETDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_

#include "afxwin.h"
#include "afxcmn.h"
#include "PictureCtrl.h"
#include <set>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HandsetDlg.h : header file
//

#define LIVE_FREEZE_STREAM			_T("Live/Freeze Image Stream")
#define SELECT_BAND_IMAGING_ON_OFF	_T("Turn On/Off Select Band Imaging")
#define STITCHED_UNSTICHED_IMAGES	_T("Stitched/Unstitched Images")
#define NEXT_COLOR					_T("Change to Next Color")
#define PREVIOUS_COLOR				_T("Change to Previous Color")
#define FRONT_LIGHTS_ON_OFF			_T("Turn On/Off Front Lights")
#define SIDE_LIGHTS_ON_OFF			_T("Turn On/Off Side Lights")
#define APPLY_MASK_ON_OFF			_T("Turn On/Off Stitch Mask")
#define APPLY_ENHANCEMENT_ON_OFF	_T("Turn On/Off Enhancement")
#define MARKERS_ON_OFF				_T("Turn On/Off Markers")
#define NEXT_ENHANCEMENT			_T("Change to Next Enhancement")
#define PREVIOUS_ENHANCEMENT		_T("Change to Previous Enhancement")
#define BLENDING_ON_OFF				_T("Turn On/Off Blending")
#define CAPTURE_IMAGE_FILE			_T("Capture Image File")

/////////////////////////////////////////////////////////////////////////////
// CHandsetDlg dialog

class CHandsetDlg : public CDialog
{
	// Construction
public:
	CHandsetDlg(CWnd* pParent = NULL);   // standard constructor

	BOOL OnInitDialog();

	// Dialog Data
	//{{AFX_DATA(CHandsetDlg)
	enum { IDD = IDD_HANDSET };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHandsetDlg)

	void DoAction(CString action);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHandsetDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CPictureCtrl m_picCtrl;
	std::vector<CString> m_AllowedActions;
	std::map<int, int> m_mapButtonLookup;

	int GetSwitchID(int button);
	
public:
	afx_msg void OnBnClickedSwitch1();
	afx_msg void OnBnClickedSwitch2();
	afx_msg void OnBnClickedSwitch3();
	afx_msg void OnBnClickedSwitch4();
	afx_msg void OnBnClickedSwitch5();
	CComboBox m_comboSwitch1;
	CComboBox m_comboSwitch2;
	CComboBox m_comboSwitch3;
	CComboBox m_comboSwitch4;
	CComboBox m_comboSwitch5;
	afx_msg LRESULT OnMicroControllerMessage(WPARAM control, LPARAM value);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	void ReloadControls(string folder);
	void SaveControls(string folder);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HANDESETDLG_H__68588E2E_CBCB_4717_A400_FC1029FC4C9A__INCLUDED_)