// CameraDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "CameraDlg.h"
#include "Timers.h"
#include <opencv2\highgui\highgui.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define sWM_GET_DEFAULT_SIZE	_T("WM_GET_DEFAULT_SIZE")  // LPARAM = SIZE*, return TRUE if handled

#define AUTO_WHITE_BRIGHTNESS 128

/////////////////////////////////////////////////////////////////////////////
// CCameraDlg dialog
CCameraDlg::CCameraDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCameraDlg::IDD, pParent)
	, m_fRedGain(1.0)
	, m_fGreenGain(1.0)
	, m_fBlueGain(1.0)
	, m_pCurrentCamera(NULL)
	, m_nBayerType(0)
	, m_bTestPatternEnabled(FALSE)
	, m_bAutoExposureEnabled(TRUE)
	, m_bAutoGainEnabled(TRUE)
	, m_nExposureSetting(0)
	, m_nGainSetting(0)
	, m_uTimerVal(0)
	, m_dGamma(1.0)
	, m_sRegisterAddress(_T(""))
	, m_sRegisterValue(_T(""))
{
	//{{AFX_DATA_INIT(CCameraDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	_bExiting = false;
	_hThread = NULL;
}

CCameraDlg::~CCameraDlg()
{
	StopCaptureThread();
}


void CCameraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCameraDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT_RED_GAIN, m_fRedGain);
	DDX_Text(pDX, IDC_EDIT_GREEN_GAIN, m_fGreenGain);
	DDX_Text(pDX, IDC_EDIT_BLUE_GAIN, m_fBlueGain);
	DDX_Control(pDX, IDC_COMBO_CAM_SELECT, m_cbCameraSelect);
	DDX_Text(pDX, IDC_EDIT_BAYER_TYPE, m_nBayerType);
	DDV_MinMaxInt(pDX, m_nBayerType, 0, 3);
	DDX_Check(pDX, IDC_CHECK_TEST_PATTERN, m_bTestPatternEnabled);
	DDX_Check(pDX, IDC_CHECK_AUTO_EXPOSURE, m_bAutoExposureEnabled);
	DDX_Check(pDX, IDC_CHECK_AUTO_GAIN, m_bAutoGainEnabled);
	DDX_Text(pDX, IDC_EDIT_EXPOSURE_TIME, m_nExposureSetting);
	DDX_Text(pDX, IDC_EDIT_GAIN, m_nGainSetting);
	DDX_Text(pDX, IDC_EDIT_GAMMA, m_dGamma);
	DDX_Text(pDX, IDC_EDIT_REGISTER, m_sRegisterAddress);
	DDX_Text(pDX, IDC_EDIT_REGISTER_VAL, m_sRegisterValue);
}


BEGIN_MESSAGE_MAP(CCameraDlg, CDialog)
	//{{AFX_MSG_MAP(CCameraDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
	ON_WM_SHOWWINDOW()
	ON_WM_TIMER()
	ON_CBN_SELCHANGE(IDC_COMBO_CAM_SELECT, &CCameraDlg::OnCbnSelchangeComboCamSelect)
	ON_EN_KILLFOCUS(IDC_EDIT_BAYER_TYPE, &CCameraDlg::OnEnKillfocusEditBayerType)
	ON_EN_KILLFOCUS(IDC_EDIT_RED_GAIN, &CCameraDlg::OnEnKillfocusEditRedGain)
	ON_EN_KILLFOCUS(IDC_EDIT_GREEN_GAIN, &CCameraDlg::OnEnKillfocusEditGreenGain)
	ON_EN_KILLFOCUS(IDC_EDIT_BLUE_GAIN, &CCameraDlg::OnEnKillfocusEditBlueGain)
	ON_BN_CLICKED(IDC_BUTTON_WHITE_CAL, &CCameraDlg::OnBnClickedButtonWhiteCal)
	ON_BN_CLICKED(IDC_CHECK_AUTO_EXPOSURE, &CCameraDlg::OnBnClickedCheckAutoExposure)
	ON_BN_CLICKED(IDC_CHECK_AUTO_GAIN, &CCameraDlg::OnBnClickedCheckAutoGain)
	ON_BN_CLICKED(IDC_CHECK_TEST_PATTERN, &CCameraDlg::OnBnClickedCheckTestPattern)
	ON_EN_KILLFOCUS(IDC_EDIT_EXPOSURE_TIME, &CCameraDlg::OnEnKillfocusEditExposureTime)
	ON_EN_KILLFOCUS(IDC_EDIT_GAIN, &CCameraDlg::OnEnKillfocusEditGain)
	ON_BN_CLICKED(IDC_BUTTON_FPN_CAL, &CCameraDlg::OnBnClickedButtonFpnCal)
	ON_BN_CLICKED(IDC_BUTTON_REGISTER_READ, &CCameraDlg::OnBnClickedButtonRegisterRead)
	ON_BN_CLICKED(IDC_BUTTON_REGISTER_WRITE, &CCameraDlg::OnBnClickedButtonRegisterWrite)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCameraDlg message handlers

BOOL CCameraDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	for (int i = 0; i < theApp.GetSystemManager()->GetNumCameras(); i++)
	{
		CString camName = CameraNames[i].c_str();
		m_cbCameraSelect.AddString(camName);
	}
	m_cbCameraSelect.SetCurSel(0);

	
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CCameraDlg::OnShowWindow(BOOL bShow, 
							   UINT nStatus)
{
	if (bShow)
	{
		OnCbnSelchangeComboCamSelect();
	}
	else
	{
		StopCaptureThread();
	}
}



void CCameraDlg::OnCbnSelchangeComboCamSelect()
{
	UpdateData();

	int selIndex = m_cbCameraSelect.GetCurSel();
	m_pCurrentCamera = theApp.GetSystemManager()->GetCameraData((CameraType)selIndex);

	m_pCurrentCamera->GetColorGains(m_fRedGain, m_fGreenGain, m_fBlueGain);
	m_nBayerType = m_pCurrentCamera->GetBayerType();
	
	_hCameraEvent = m_pCurrentCamera->GetCaptureEvent();

	m_nExposureSetting = m_pCurrentCamera->GetExposure();
	m_bAutoExposureEnabled = m_pCurrentCamera->IsAutoExposureEnabled();
	GetDlgItem(IDC_EDIT_EXPOSURE_TIME)->EnableWindow(!m_bAutoExposureEnabled);
	
	m_nGainSetting = m_pCurrentCamera->GetGain();

	bool isFront = ((CameraType)selIndex == eFront);

	bool bAutoLights = ((isFront && theApp.GetSystemManager()->GetLightControl()->GetFrontLightsAuto())
		|| (!isFront && theApp.GetSystemManager()->GetLightControl()->GetSideLightsAuto()));
	m_bAutoGainEnabled = bAutoLights || m_pCurrentCamera->IsAutoGainEnabled();
	GetDlgItem(IDC_EDIT_GAIN)->EnableWindow(!m_bAutoGainEnabled);

	m_dGamma = m_pCurrentCamera->GetGamma();
	
	int board = m_pCurrentCamera->GetUSBCameraNum();
	theApp.GetSystemManager()->GetMicroController()->SetShutterMonitor(m_bAutoExposureEnabled ? board : 0);
	theApp.GetSystemManager()->GetMicroController()->SetGainMonitor(m_bAutoGainEnabled ? board : 0);

	StopCaptureThread();

	// Clear the image
	CRect rect;
	GetDlgItem(IDC_CAM_IMAGE)->GetWindowRect(&rect);
	m_cvImg.setTo(0);
	ScreenToClient(&rect);
	InvalidateRect(rect, FALSE);

	// Create the update thread
	DWORD dwThreadID;
	_hThread = CreateThread(
		NULL,         // default security attributes
		0,            // default stack size
		(LPTHREAD_START_ROUTINE)&CCameraDlg::ThreadProc,
		(LPVOID)this,         // no thread function arguments
		0,            // default creation flags
		&dwThreadID); // receive thread identifier

	SetEvent(_hCameraEvent);

	// For this demo, we specify an interval that won't overlap 
	// with the window timer.
	if (m_uTimerVal != 0)
	{
		KillTimer(m_uTimerVal);
	}
	m_uTimerVal = SetTimer(IDT_TIMER_CAM_REFRESH, 1000, NULL);

	UpdateData(FALSE);
}

void CCameraDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rect;
	GetDlgItem(IDC_CAM_IMAGE)->GetWindowRect(&rect);
	ScreenToClient(&rect);

	// TODO: add draw code for native data here
	int height = m_cvImg.rows;
	int width = m_cvImg.cols;
	uchar buffer[sizeof(BITMAPINFOHEADER) + 1024];
	BITMAPINFO* bmi = (BITMAPINFO*)buffer;
	FillBitmapInfo(bmi, width, height, 8 * m_cvImg.channels(), 0);
	SetDIBitsToDevice(dc.GetSafeHdc(), rect.left, rect.top, width,
						height, 0, 0, 0, height, m_cvImg.data, bmi,
						DIB_RGB_COLORS);

	double red, green, blue;
	CString value;

	m_pCurrentCamera->GetLastChannelValues(red, green, blue);
	value.Format(_T("%03d,%03d,%03d"), (int)red, (int)green, (int)blue);
	GetDlgItem(IDC_EDIT_CAMERA_AVG)->SetWindowText(value);

	m_pCurrentCamera->GetLastSaturationValues(red, green, blue);
	value.Format(_T("%03d,%03d,%03d"), (int)round(100.0*red), (int)round(100.0*green), (int)round(100.0*blue));
	GetDlgItem(IDC_EDIT_CAMERA_SAT)->SetWindowText(value);
}

void CCameraDlg::FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin)
{
	assert(bmi && width >= 0 && height >= 0 && (bpp == 8 || bpp == 24 || bpp == 32));

	BITMAPINFOHEADER* bmih = &(bmi->bmiHeader);

	memset(bmih, 0, sizeof(*bmih));
	bmih->biSize = sizeof(BITMAPINFOHEADER);
	bmih->biWidth = width;
	bmih->biHeight = origin ? abs(height) : -abs(height);
	bmih->biPlanes = 1;
	bmih->biBitCount = (unsigned short)bpp;
	bmih->biCompression = BI_RGB;

	if (bpp == 8)
	{
		RGBQUAD* palette = bmi->bmiColors;

		for (int i = 0; i < 256; i++)
		{
			palette[i].rgbBlue = palette[i].rgbGreen = palette[i].rgbRed = (BYTE)i;
			palette[i].rgbReserved = 0;
		}
	}
}

void CCameraDlg::StopCaptureThread()
{
	// Restart the update thread
	if (_hThread != NULL)
	{
		_bExiting = true;
		WaitForSingleObject(_hThread, 1000);
		_bExiting = false;
	}

	if (theApp.GetSystemManager() != NULL)
	{
		theApp.GetSystemManager()->GetMicroController()->SetShutterMonitor(0);
	}
}

UINT CCameraDlg::CaptureLoop()
{
	while (!_bExiting)
	{
		DWORD dwEvent = WaitForSingleObject(
			_hCameraEvent, 
			500);

		if (dwEvent == WAIT_TIMEOUT)
		{
			continue;
		}

		ResetEvent(_hCameraEvent);

		try
		{
			CRect rect;
			GetDlgItem(IDC_CAM_IMAGE)->GetWindowRect(&rect);
			// Make the width a factor of 4
			int width = rect.Width();
			int height = rect.Height();
			int rem = width % 4;
			if (rem != 0)
			{
				width -= rem;
				float scale = 1.0 * width / rect.Width();
				height = (int)(scale * rect.Height());
			}
			resize(m_pCurrentCamera->GetCameraImage(), m_cvImg, Size(width, height));

			ScreenToClient(&rect);
			InvalidateRect(rect, FALSE);
		}
		catch (...)
		{
			// TODO add log statement
		}

	}

	return 0;
}

void CCameraDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (m_pCurrentCamera)
	{
		int board = m_pCurrentCamera->GetUSBCameraNum();
		
		if (m_bAutoExposureEnabled)
		{
			int nShutterValue = theApp.GetSystemManager()->GetMicroController()->GetShutterValue(board);
			CString value;
			value.Format(_T("%d"), nShutterValue);
			GetDlgItem(IDC_EDIT_EXPOSURE_TIME)->SetWindowTextW(value);
		}

		if (m_bAutoGainEnabled)
		{
			int nGainValue = theApp.GetSystemManager()->GetMicroController()->GetGainValue(board);
			CString value;
			value.Format(_T("%d"), nGainValue);
			GetDlgItem(IDC_EDIT_GAIN)->SetWindowTextW(value);
		}
	}
}

void CCameraDlg::OnEnKillfocusEditBayerType()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		m_pCurrentCamera->SetBayerType(m_nBayerType);
	}
}


void CCameraDlg::OnEnKillfocusEditRedGain()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		m_pCurrentCamera->SetColorGains(m_fRedGain, m_fGreenGain, m_fBlueGain);
	}
}


void CCameraDlg::OnEnKillfocusEditGreenGain()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		m_pCurrentCamera->SetColorGains(m_fRedGain, m_fGreenGain, m_fBlueGain);
	}
}


void CCameraDlg::OnEnKillfocusEditBlueGain()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		m_pCurrentCamera->SetColorGains(m_fRedGain, m_fGreenGain, m_fBlueGain);
	}
}


void CCameraDlg::OnBnClickedButtonWhiteCal()
{
	if (m_pCurrentCamera)
	{
		// Ideal pink color for colon model
		//double calBlueComp = 0.54;
		//double calGreenComp = 1.0;
		//double calRedComp = 1.71;
		double calBlueComp = 1.0;
		double calGreenComp = 1.0;
		double calRedComp = 1.0;
		m_pCurrentCamera->CalibrateWhiteBalance(calRedComp, calGreenComp, calBlueComp);

		m_pCurrentCamera->GetColorGains(m_fRedGain, m_fGreenGain, m_fBlueGain);

		UpdateData(FALSE);
	}
}

void CCameraDlg::OnEnKillfocusEditExposureTime()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		m_pCurrentCamera->SetExposure(m_nExposureSetting);
	}
}

void CCameraDlg::OnEnKillfocusEditGain()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		m_pCurrentCamera->SetGain(m_nGainSetting);
	}
}

void CCameraDlg::OnBnClickedCheckAutoExposure()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		m_pCurrentCamera->EnableAutoExposure(m_bAutoExposureEnabled);

		int board = m_pCurrentCamera->GetUSBCameraNum();
		theApp.GetSystemManager()->GetMicroController()->SetShutterMonitor(m_bAutoExposureEnabled ? board : 0);
		GetDlgItem(IDC_EDIT_EXPOSURE_TIME)->EnableWindow(!m_bAutoExposureEnabled);
	}
}

void CCameraDlg::OnBnClickedCheckAutoGain()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		m_pCurrentCamera->EnableAutoGain(m_bAutoGainEnabled);
		
		int board = m_pCurrentCamera->GetUSBCameraNum();
		theApp.GetSystemManager()->GetMicroController()->SetGainMonitor(m_bAutoGainEnabled ? board : 0);
		GetDlgItem(IDC_EDIT_GAIN)->EnableWindow(!m_bAutoGainEnabled);
	}
}

void CCameraDlg::OnBnClickedCheckTestPattern()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		int board = m_pCurrentCamera->GetUSBCameraNum();
		theApp.GetSystemManager()->GetMicroController()->SetTestPattern(board, m_bTestPatternEnabled);
	}
}



void CCameraDlg::OnBnClickedButtonFpnCal()
{
	if (m_pCurrentCamera)
	{
		m_pCurrentCamera->CalibrateFPN();
	}
}



void CCameraDlg::OnBnClickedButtonRegisterRead()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		int board = m_pCurrentCamera->GetUSBCameraNum();

		wchar_t *end = NULL;
		long registerAddress = wcstol(m_sRegisterAddress, &end, 16);
		unsigned short registerValue;
		theApp.GetSystemManager()->GetMicroController()->ReadCameraRegister(board, registerAddress, registerValue);

		m_sRegisterValue.Format(_T("%x"), registerValue);
		UpdateData(FALSE);
	}
}


void CCameraDlg::OnBnClickedButtonRegisterWrite()
{
	UpdateData();

	if (m_pCurrentCamera)
	{
		int board = m_pCurrentCamera->GetUSBCameraNum();

		wchar_t *end = NULL;
		long registerAddress = wcstol(m_sRegisterAddress, &end, 16);
		long registerValue = wcstol(m_sRegisterValue, &end, 16);
		theApp.GetSystemManager()->GetMicroController()->WriteCameraRegister(board, registerAddress, registerValue, true);
	}
}
