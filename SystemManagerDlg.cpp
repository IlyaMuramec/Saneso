// SystemManagerDlg.cpp : implem entation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "SystemManagerDlg.h"
#include "SystemManager.h"
#include "SplashScreen.h"
#include "SelectItemDlg.h"
#include "Version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSystemManagerDlg dialog


CSystemManagerDlg::CSystemManagerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSystemManagerDlg::IDD, pParent)
	, m_sVersion(_T("Not Connected"))
	, m_sStatus(_T(""))
	, m_sSWVersion(_T(""))
{
	//{{AFX_DATA_INIT(CSystemManagerDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pSystemManager = NULL;

	m_sSWVersion = _T(STRFILEVER);
	m_sSWVersion.Replace(_T(", "), _T("."));
}

CSystemManagerDlg::~CSystemManagerDlg()
{
}

void CSystemManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSystemManagerDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_LED_MOSAIC, m_ledMosaic);
	DDX_Control(pDX, IDC_LED_FRONT_CAM, m_ledFrontCam);
	DDX_Control(pDX, IDC_LED_TOP_CAM, m_ledTopCam);
	DDX_Control(pDX, IDC_LED_BOTTOM_CAM, m_ledBottomCam);
	DDX_Control(pDX, IDC_LED_LEFT_CAM, m_ledLeftCam);
	DDX_Control(pDX, IDC_LED_RIGHT_CAM, m_ledRightCam);
	DDX_Control(pDX, IDC_LED_CONTROLLER, m_ledController);
	DDX_Text(pDX, IDC_EDIT_VERSION, m_sVersion);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT_STATUS, m_sStatus);
	DDX_Text(pDX, IDC_EDIT_SW_VERSION, m_sSWVersion);
}


BEGIN_MESSAGE_MAP(CSystemManagerDlg, CDialog)
	//{{AFX_MSG_MAP(CSystemManagerDlg)
	// NOTE: the ClassWizard will add message map macros here
	ON_WM_DRAWITEM()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_REGISTERED_MESSAGE(UWM_UPDATE_SYSTEM_STATUS_MSG, &CSystemManagerDlg::OnUpdateSystemStatusMessage)
	ON_REGISTERED_MESSAGE(UWM__MSG_RESET_USB_CHIPS, &CSystemManagerDlg::OnResetUSBChipsMessage)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CSystemManagerDlg::OnBnClickedButtonReset)
	ON_BN_CLICKED(IDC_SELECT_ALL_CAM, &CSystemManagerDlg::OnBnClickedSelectAllCam)
	ON_BN_CLICKED(IDC_BUTTON_RESET_MICROCONTROLLER, &CSystemManagerDlg::OnBnClickedButtonResetMicroController)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_DEBUG_IMAGES, &CSystemManagerDlg::OnBnClickedButtonSaveDebugImages)
	ON_BN_CLICKED(IDC_BUTTON_RESET_USB_CHIPS, &CSystemManagerDlg::OnBnClickedButtonResetUsbChips)
	ON_BN_CLICKED(IDC_CHECK_LOG_DEBUG, &CSystemManagerDlg::OnBnClickedCheckLogDebug)
	ON_BN_CLICKED(IDC_BUTTON_CAL_WHITE, &CSystemManagerDlg::OnBnClickedButtonCalWhite)
	ON_BN_CLICKED(IDC_BUTTON_CAL_FPN, &CSystemManagerDlg::OnBnClickedButtonCalFPN)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSystemManagerDlg message handlers

BOOL CSystemManagerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pSystemManager = theApp.GetSystemManager();
	m_pSystemManager->SetSystemManagerDlg(this);

	SetupLEDControl(&m_ledMosaic);
	SetupLEDControl(&m_ledFrontCam);
	SetupLEDControl(&m_ledTopCam);
	SetupLEDControl(&m_ledBottomCam);
	SetupLEDControl(&m_ledLeftCam);
	SetupLEDControl(&m_ledRightCam);
	SetupLEDControl(&m_ledController);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CSystemManagerDlg::SetupLEDControl(CLedButton *led)
{
	led->SetLedStatesNumber(LIGHT_STATE_SENTINEL);
	led->SetIcon(GRAY_LIGHT, IDI_GRAY_LED_ICON);
	led->SetIcon(GREEN_LIGHT, IDI_GREEN_LED_ICON);
	led->SetIcon(YELLOW_LIGHT, IDI_YELLOW_LED_ICON);
	led->SetIcon(RED_LIGHT, IDI_RED_LED_ICON);
	led->SetIcon(BLUE_LIGHT, IDI_BLUE_LED_ICON);
	led->SetLedState(0);
}

void CSystemManagerDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CSystemManagerDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		UpdateData(FALSE);
	}
	else
	{
	}
}

LRESULT CSystemManagerDlg::OnUpdateSystemStatusMessage(WPARAM p1, LPARAM p2)
{
	CString str;

	SystemStatus *pSystemStatus = (SystemStatus*)p1;

	str.Format(_T("%0.3f"), pSystemStatus->m_dFPSMosaic);
	GetDlgItem(IDC_EDIT_FPS_MOSAIC)->SetWindowTextW(str);

	str.Format(_T("%0.3f"), pSystemStatus->m_dFPSFrontCamera);
	GetDlgItem(IDC_EDIT_FPS_FRONT_CAMERA)->SetWindowTextW(str);

	str.Format(_T("%0.3f"), pSystemStatus->m_dFPSTopCamera);
	GetDlgItem(IDC_EDIT_FPS_TOP_CAMERA)->SetWindowTextW(str);

	str.Format(_T("%0.3f"), pSystemStatus->m_dFPSBottomCamera);
	GetDlgItem(IDC_EDIT_FPS_BOTTOM_CAMERA)->SetWindowTextW(str);

	str.Format(_T("%0.3f"), pSystemStatus->m_dFPSLeftCamera);
	GetDlgItem(IDC_EDIT_FPS_LEFT_CAMERA)->SetWindowTextW(str);

	str.Format(_T("%0.3f"), pSystemStatus->m_dFPSRightCamera);
	GetDlgItem(IDC_EDIT_FPS_RIGHT_CAMERA)->SetWindowTextW(str);

	str.Format(_T("%0.2f"), pSystemStatus->m_dControllerHeartBeat);
	GetDlgItem(IDC_EDIT_HEART_BEAT)->SetWindowTextW(str);

	m_sVersion = pSystemStatus->m_ControllerVersion.c_str();
	m_sStatus = pSystemStatus->m_ControllerStatusString.c_str();

	m_ledMosaic.SetLedState((LedState)pSystemStatus->m_MosaicStatus);
	m_ledFrontCam.SetLedState((LedState)pSystemStatus->m_CamerasStatus[eFront]);
	m_ledTopCam.SetLedState((LedState)pSystemStatus->m_CamerasStatus[eTop]);
	m_ledBottomCam.SetLedState((LedState)pSystemStatus->m_CamerasStatus[eBottom]);
	m_ledLeftCam.SetLedState((LedState)pSystemStatus->m_CamerasStatus[eLeft]);
	m_ledRightCam.SetLedState((LedState)pSystemStatus->m_CamerasStatus[eRight]);
	m_ledController.SetLedState((LedState)pSystemStatus->m_ControllerStatus);

	GetDlgItem(IDC_FRONT_USB_VERSION)->SetWindowText(
		CString(m_pSystemManager->GetUSBeader()->GetFWVersion(eFront).c_str()));
	GetDlgItem(IDC_LEFT_USB_VERSION)->SetWindowText(
		CString(m_pSystemManager->GetUSBeader()->GetFWVersion(eLeft).c_str()));
	GetDlgItem(IDC_RIGHT_USB_VERSION)->SetWindowText(
		CString(m_pSystemManager->GetUSBeader()->GetFWVersion(eRight).c_str()));
	GetDlgItem(IDC_TOP_USB_VERSION)->SetWindowText(
		CString(m_pSystemManager->GetUSBeader()->GetFWVersion(eTop).c_str()));
	GetDlgItem(IDC_BOTTOM_USB_VERSION)->SetWindowText(
		CString(m_pSystemManager->GetUSBeader()->GetFWVersion(eBottom).c_str()));

	UpdateData(FALSE);

	return 0;
}

LRESULT CSystemManagerDlg::OnResetUSBChipsMessage(WPARAM p1, LPARAM p2)
{
	OnBnClickedButtonResetUsbChips();

	return 0;
}

void CSystemManagerDlg::OnBnClickedButtonReset()
{
	unsigned int mask = GetCameraSelectMask();
	m_pSystemManager->ResetCameras(mask);
}

void CSystemManagerDlg::OnBnClickedButtonInitialize()
{
	unsigned int mask = GetCameraSelectMask();
	m_pSystemManager->InitializeCameras(mask);
}

void CSystemManagerDlg::OnBnClickedSelectAllCam()
{

	BOOL bChecked = IsDlgButtonChecked(IDC_SELECT_ALL_CAM);

	((CButton*)GetDlgItem(IDC_SELECT_FRONT_CAM))->SetCheck(bChecked);
	((CButton*)GetDlgItem(IDC_SELECT_TOP_CAM))->SetCheck(bChecked);
	((CButton*)GetDlgItem(IDC_SELECT_LEFT_CAM))->SetCheck(bChecked);
	((CButton*)GetDlgItem(IDC_SELECT_RIGHT_CAM))->SetCheck(bChecked);
	((CButton*)GetDlgItem(IDC_SELECT_BOTTOM_CAM))->SetCheck(bChecked);
}

unsigned int CSystemManagerDlg::GetCameraSelectMask()
{
	unsigned int mask = 0;

	if (IsDlgButtonChecked(IDC_SELECT_FRONT_CAM))
	{
		mask |= (1 << eFront);
	}

	if (IsDlgButtonChecked(IDC_SELECT_TOP_CAM))
	{
		mask |= (1 << eTop);
	}

	if (IsDlgButtonChecked(IDC_SELECT_LEFT_CAM))
	{
		mask |= (1 << eLeft);
	}

	if (IsDlgButtonChecked(IDC_SELECT_RIGHT_CAM))
	{
		mask |= (1 << eRight);
	}

	if (IsDlgButtonChecked(IDC_SELECT_BOTTOM_CAM))
	{
		mask |= (1 << eBottom);
	}

	return mask;
}


void CSystemManagerDlg::OnBnClickedButtonResetMicroController()
{
	theApp.GetSystemManager()->GetMicroController()->SendReset();
}


void CSystemManagerDlg::OnBnClickedButtonSaveDebugImages()
{
	CSelectItemDlg SelectItemDlg;
	SelectItemDlg.SetTitle(_T("Select Location"));
	SelectItemDlg.IsLoading(false);
	CString configFolder = _T("C:\\NGE\\DebugImages\\");
	SelectItemDlg.SetSearchFolder(configFolder);
	SelectItemDlg.DisplayFolders(TRUE);
	if (SelectItemDlg.DoModal() == IDOK)
	{
		CString selection = SelectItemDlg.GetSelectedItem();
		CString folder = configFolder + selection + _T("\\");
		CStringA folderA = folder;
		CreateDirectory(folder, NULL);

		theApp.GetSystemManager()->DumpProcTimes(folderA.GetBuffer());

		theApp.GetSystemManager()->GetLightControl()->SaveAutoLightData(folderA.GetBuffer());

#ifdef USE_USB_CAMERA
		theApp.GetSystemManager()->GetUSBeader()->SaveNextTransfer(folderA.GetBuffer());
#else
		theApp.GetSystemManager()->GetPentekReader()->SaveNextTransfer();
#endif
	}
}

void CSystemManagerDlg::OnBnClickedButtonResetUsbChips()
{
	GetDlgItem(IDC_BUTTON_RESET_USB_CHIPS)->EnableWindow(FALSE);

	CSplashThread* pSplashThread = theApp.GetSplashThread();
	theApp.GetSystemManager()->ResetUSBChips(pSplashThread);

	GetDlgItem(IDC_BUTTON_RESET_USB_CHIPS)->EnableWindow(TRUE);
}


void CSystemManagerDlg::OnBnClickedCheckLogDebug()
{
	CButton *pCheck = (CButton *)GetDlgItem(IDC_CHECK_LOG_DEBUG);

	log4cxx::LoggerPtr logger = log4cxx::Logger::getRootLogger();
	if (pCheck->GetCheck())
	{
		logger->setLevel(log4cxx::Level::getDebug());
	}
	else
	{
		logger->setLevel(log4cxx::Level::getInfo());
	}
}


void CSystemManagerDlg::OnBnClickedButtonCalWhite()
{
	unsigned int mask = GetCameraSelectMask();
	m_pSystemManager->CalibrateCamerasWhite(mask);
}


void CSystemManagerDlg::OnBnClickedButtonCalFPN()
{
	unsigned int mask = GetCameraSelectMask();
	m_pSystemManager->CalibrateCamerasFPN(mask, theApp.GetSplashThread());
}
