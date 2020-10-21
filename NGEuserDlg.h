// SettingsDlg.h : header file
//

#if !defined(AFX_SETTINGSDLG_H__7B91BF27_DC47_47B9_B7A6_317E734B5341__INCLUDED_)
#define AFX_SETTINGSDLG_H__7B91BF27_DC47_47B9_B7A6_317E734B5341__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"
#include "ScreenSelectDlg.h"
#include "ProcessingDlg.h"
#include "CameraDlg.h"
#include "FPGADlg.h"
#include "MicroControlDlg.h"
#include "LightingDlg.h"
#include "MainDlg.h"
#include "PictureCtrl.h"
#include "HandsetDlg.h"
#include "EnhancementDlg.h"
#include "SystemManagerDlg.h"
#include "SelfCalibrationDlg.h"
#include "SignalOutputDlg.h"
#include "CefBrowserDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CNGEuserDlg dialog

class CNGEuserDlg : public CDialog
{
// Construction
public:
	CNGEuserDlg(CWnd* pParent = NULL);	// standard constructor

	CScreenSelectDlg m_dScreenSelectDlg;
	CMainDlg m_dMainDlg;
	CProcessingDlg m_dProcessingDlg;
	CCameraDlg m_dCameraDlg;
	CFPGADlg m_dFPGADlg;
	CMicroControlDlg m_dMicroControlDlg;
	CLightingDlg m_dLightingDlg;
	CHandsetDlg m_dHandsetDlg;
	CEnhancementDlg m_dEnhancementDlg;
	CSystemManagerDlg m_dSystemManagerDlg;
	CSelfCalibrationDlg m_dSelfCalibrationDlg;
	CSignalOutputDlg m_dSignalOutputDlg;
	CCefBrowserDlg m_dCefBrowserDlg;

	void ShowWindowNumber(int number);
	void RedrawDisplay();
	void UpdateTitle();
	void ReloadControls(string folder);
	void SaveControls(string folder);

	void CaptureImage(string file);
	void DisplayImage(string file);
	void CaptureVideo(string file);
	void StopCapture();
	void PlaybackFile(string file);
	void DisplayVideo(string file);

	void AddTabs(bool advanced);
	void Close();

// Dialog Data
	//{{AFX_DATA(CNGEuserDlg)
	enum { IDD = IDD_NGE_GUI };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNGEuserDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CNGEuserDlg)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	LRESULT OnSelectScreen(WPARAM control, LPARAM value);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	static log4cxx::LoggerPtr logger;

	bool _bExiting;
	CRect _rSettingsRect;
	HANDLE _hStitchThread;
	HANDLE *_hCameraEvents;
	int _nNumCameras;
	int _nSettingsScreen;
	std::string m_sCaptureFile;
	std::string m_sDisplayFile;
	std::string m_sVideoFile;
	std::string m_sPlaybackFile;

	cv::Mat _cvMosaicImg;
	CPictureCtrl m_picCtrl;
	std::vector<CDialog *> _vDlgPointers;

	LONGLONG _perfFreq;
	LONGLONG _lastFrameTime;
	double _frameRate;

	static UINT ThreadStitchProc(LPVOID param) 
				{ return ((CNGEuserDlg*)param)->StitchLoop(); }

	UINT StitchLoop();

	void DrawMosaic(CDC &dc, CRect &rect);
	void FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin);

	void AddAdvancedTabs();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETTINGSDLG_H__7B91BF27_DC47_47B9_B7A6_317E734B5341__INCLUDED_)
