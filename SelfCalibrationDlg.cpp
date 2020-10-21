// SelfCalibrationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "SelfCalibrationDlg.h"
#include "SystemManager.h"
#include "SplashScreen.h"
#include "SelectItemDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelfCalibrationDlg dialog


CSelfCalibrationDlg::CSelfCalibrationDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSelfCalibrationDlg::IDD, pParent)
	, m_sLastCalTime(_T(""))
{
	//{{AFX_DATA_INIT(CSelfCalibrationDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pSelfCalibration = NULL;
}

CSelfCalibrationDlg::~CSelfCalibrationDlg()
{
}

void CSelfCalibrationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelfCalibrationDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_LED_LAST_CAL, m_ledLastCal);
	DDX_Control(pDX, IDC_LED_CAL_CAMERAS_OK, m_ledCameras);
	DDX_Control(pDX, IDC_LED_CAL_CUP_CHECK, m_ledCalCup);
	DDX_Control(pDX, IDC_LED_CAL_LIGHTS_OK, m_ledLights);
	DDX_Control(pDX, IDC_LED_CAL_FPN, m_ledFPN);
	DDX_Control(pDX, IDC_LED_CAL_WB, m_ledWB);
	DDX_Control(pDX, IDC_LED_CAL_OVERALL, m_ledOverall);

	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT_LAST_CAL, m_sLastCalTime);
}


BEGIN_MESSAGE_MAP(CSelfCalibrationDlg, CDialog)
	//{{AFX_MSG_MAP(CSelfCalibrationDlg)
	// NOTE: the ClassWizard will add message map macros here
	ON_WM_DRAWITEM()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_REGISTERED_MESSAGE(UWM_UPDATE_SELF_CALIBRATION_MSG, &CSelfCalibrationDlg::OnUpdateSelfCalibrationMessage)
	ON_REGISTERED_MESSAGE(UWM_LAST_SELF_CALIBRATION_MSG, &CSelfCalibrationDlg::OnLastSelfCalibrationMessage)
	ON_BN_CLICKED(IDC_BUTTON_START_SELF_CAL, &CSelfCalibrationDlg::OnBnClickedButtonStartSelfCal)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelfCalibrationDlg message handlers

BOOL CSelfCalibrationDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetupLEDControl(&m_ledLastCal);
	SetupLEDControl(&m_ledCameras);
	SetupLEDControl(&m_ledCalCup);
	SetupLEDControl(&m_ledLights);
	SetupLEDControl(&m_ledFPN);
	SetupLEDControl(&m_ledWB);
	SetupLEDControl(&m_ledOverall);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CSelfCalibrationDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CSelfCalibrationDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		LOGFONT lf;
		CFont *currentFont = GetDlgItem(IDC_BUTTON_START_SELF_CAL)->GetFont();
		currentFont->GetLogFont(&lf);
		lf.lfHeight = 20;
		lf.lfWeight = 600;
		m_StartButtonFont.DeleteObject();
		m_StartButtonFont.CreateFontIndirect(&lf);
		GetDlgItem(IDC_BUTTON_START_SELF_CAL)->SetFont(&m_StartButtonFont);

		m_pSelfCalibration = theApp.GetSystemManager()->GetSelfCalibration();
		m_pSelfCalibration->SetSelfCalibrationDlg(this);

		UpdateData(FALSE);
	}
	else
	{
	}
}

void CSelfCalibrationDlg::SetupLEDControl(CLedButton *led)
{
	led->SetLedStatesNumber(LIGHT_STATE_SENTINEL);
	led->SetIcon(GRAY_LIGHT, IDI_GRAY_LED_ICON);
	led->SetIcon(GREEN_LIGHT, IDI_GREEN_LED_ICON);
	led->SetIcon(YELLOW_LIGHT, IDI_YELLOW_LED_ICON);
	led->SetIcon(RED_LIGHT, IDI_RED_LED_ICON);
	led->SetIcon(BLUE_LIGHT, IDI_BLUE_LED_ICON);
	led->SetLedState(0);
}

LRESULT CSelfCalibrationDlg::OnUpdateSelfCalibrationMessage(WPARAM p1, LPARAM p2)
{
	CString str;

	SelfCalibrationStatus *pSelfCalStatus = (SelfCalibrationStatus*)p1;

	m_ledCameras.SetLedState((LedState)pSelfCalStatus->m_CamerasCalStatus);
	m_ledCalCup.SetLedState((LedState)pSelfCalStatus->m_CalCupCalStatus);
	m_ledLights.SetLedState((LedState)pSelfCalStatus->m_LightsCalStatus);
	m_ledFPN.SetLedState((LedState)pSelfCalStatus->m_FPNCalStatus);
	m_ledWB.SetLedState((LedState)pSelfCalStatus->m_WBCalStatus);
	m_ledOverall.SetLedState((LedState)pSelfCalStatus->m_OverallCalStatus);

	if (pSelfCalStatus->m_OverallCalStatus != eUnknown)
		GetDlgItem(IDC_BUTTON_START_SELF_CAL)->EnableWindow(TRUE);
	
	UpdateData(FALSE);

	return 0;
}

LRESULT CSelfCalibrationDlg::OnLastSelfCalibrationMessage(WPARAM p1, LPARAM p2)
{
	CString str;

	SelfCalibrationStatus *pSelfCalStatus = (SelfCalibrationStatus*)p1;

	m_ledLastCal.SetLedState((LedState)pSelfCalStatus->m_OverallCalStatus);

	if (pSelfCalStatus->m_OverallCalStatus != eUnknown)
	{
		time_t rawtime;
		struct tm * timeinfo = localtime(&pSelfCalStatus->m_Time);
		m_sLastCalTime.Format(_T("%02d/%02d/%04d %02d:%02d:%02d"),
			timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_year + 1900,
			timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	}
	else
	{
		m_sLastCalTime = "";
	}

	UpdateData(FALSE);

	return 0;
}


void CSelfCalibrationDlg::OnBnClickedButtonStartSelfCal()
{
	GetDlgItem(IDC_BUTTON_START_SELF_CAL)->EnableWindow(FALSE);

	if (!m_pSelfCalibration->StartCalibration())
	{
		GetDlgItem(IDC_BUTTON_START_SELF_CAL)->EnableWindow(TRUE);
	}
}
