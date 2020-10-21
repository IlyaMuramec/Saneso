// SettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NGEuserApp.h"
#include "NGEuserDlg.h"
#include "Monitors.h"
#include "MultiMonitor.h"
#include "MonitorDC.h"
#include "ProcessorBoxSettings.h"
#include "SplashScreen.h"
#include "SelectItemDlg.h"
#include <opencv2\highgui\highgui.hpp>
#include <windows.h>
#include <stdio.h>

log4cxx::LoggerPtr  CNGEuserDlg::logger(log4cxx::Logger::getLogger("NGEuserDlg"));

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int ModalMessageDisplay::ShowMessageBox(CString text, CString caption, UINT uType)
{
	return theApp.ShowMessageBox(text, caption, uType);
}


/////////////////////////////////////////////////////////////////////////////
// CNGEuserDlg dialog

CNGEuserDlg::CNGEuserDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNGEuserDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNGEuserDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	_bExiting = false;
	_nSettingsScreen = 0;

	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	_perfFreq = li.QuadPart;
	_lastFrameTime = 0;
	_frameRate = 0;
}

void CNGEuserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNGEuserDlg)
	DDX_Control(pDX, IDC_LOGO, m_picCtrl);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNGEuserDlg, CDialog)
	//{{AFX_MSG_MAP(CNGEuserDlg)
	ON_WM_PAINT()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_QUERYDRAGICON()
	ON_WM_SHOWWINDOW()
	ON_MESSAGE(WM_UPDATE_SCREEN, &CNGEuserDlg::OnSelectScreen)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNGEuserDlg message handlers

BOOL CNGEuserDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	theApp.GetSplashThread()->SetNGEuserDlg(this);

	// Create the screen selection dialog
	m_dScreenSelectDlg.Create(IDD_SCREEN_SELECT, this);
	
	SetForegroundWindow();
	ShowWindow(SW_MAXIMIZE);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Load the logo into the picture control
//	m_picCtrl.LoadFromResource(NULL, MAKEINTRESOURCE(IDB_SANESO_PNG), _T("PNG"));

	// Create the child windows for the main window class
	
	
	m_dCefBrowserDlg.Create(IDD_CEF_BROWSER, this);
	m_dMainDlg.Create(IDD_MAIN, this);	
	m_dProcessingDlg.Create(IDD_PROCESSING, this);
	m_dSystemManagerDlg.Create(IDD_SYSTEM_MANAGER, this);
	m_dCameraDlg.Create(IDD_CAMERA, this);	
	m_dLightingDlg.Create(IDD_LIGHTING, this);
	m_dHandsetDlg.Create(IDD_HANDSET, this);
	m_dEnhancementDlg.Create(IDD_ENHANCEMENT, this);
	m_dSelfCalibrationDlg.Create(IDD_SELF_CALIBRATION, this);
	m_dMicroControlDlg.Create(IDD_MICROCONTROL, this);
	m_dSignalOutputDlg.Create(IDD_SIGNALOUTPUT, this);
	
#ifndef USE_USB_CAMERA
	m_dFPGADlg.Create(IDD_FPGA, this);
#endif

	AddTabs(theApp.IsLoggedIn());

	// create Cef Browser
	{
		// get rect
		CRect rect;
		m_dCefBrowserDlg.GetClientRect(&rect);
		TCHAR szDirectory[MAX_PATH] = _T("");
		::GetCurrentDirectory(sizeof(szDirectory) - 1, szDirectory);
		BOOL result = theApp.CreateCefBrowser(m_dCefBrowserDlg.GetSafeHwnd(), rect, CString(szDirectory) + _T("/main.html"));
		if (!result)
			AfxMessageBox(_T("Failed CreateCefBrowser in dialog"));
	}

	// Create two event objects
	_nNumCameras = (int)theApp.GetSystemManager()->GetNumCameras();
	_hCameraEvents = new HANDLE[_nNumCameras];
	for (int i = 0; i < _nNumCameras; i++)
	{
		_hCameraEvents[i] = theApp.GetSystemManager()->GetCameraData((CameraType)i)->GetUpdateEvent();
	}
	// Create a thread
	DWORD dwThreadID;
	_hStitchThread = CreateThread(
		NULL,         // default security attributes
		0,            // default stack size
		(LPTHREAD_START_ROUTINE)&CNGEuserDlg::ThreadStitchProc,
		(LPVOID)this,         // no thread function arguments
		0,            // default creation flags
		&dwThreadID); // receive thread identifier

	UpdateTitle();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CNGEuserDlg::OnCancel()
{
	LOG4CXX_DEBUG(logger, "Exit initiated");

	int retVal = theApp.ShowMessageBox(_T("Exit the application?"), _T("Warning"), MB_OKCANCEL | MB_TOPMOST);
	if (retVal == IDCANCEL)
	{
		return;
	}

	Close();
}

void CNGEuserDlg::Close()
{
	LOG4CXX_INFO(logger, "Exiting Application");

	theApp.CloseCefBrowser();

	// End the Capture to clean up the DMA
	_bExiting = true;
	WaitForSingleObject(_hStitchThread, 1000);

	LOG4CXX_INFO(logger, "Stitch thread is done");

	// Stop any capture thread on the Camera Dlg
	m_dCameraDlg.StopCaptureThread();

	LOG4CXX_INFO(logger, "Camera display thead is done");

	theApp.GetSystemManager()->Close();

	LOG4CXX_INFO(logger, "System Manager is done");

	// Draw black on second monitor
	CMonitor monitor;
	CMonitors monitors;

	for (int i = 0; i < monitors.GetCount(); i++)
	{
		monitor = monitors.GetMonitor(i);
		if (!monitor.IsPrimaryMonitor())
		{
			CRect rect;
			monitor.GetMonitorRect(&rect);
			CMonitorDC dc(&monitor);

			FillRect(dc, &rect, (HBRUSH)(RGB(0, 0, 0)));
		}
	}

	LOG4CXX_INFO(logger, "Close is done");

	CDialog::OnCancel();
}


void CNGEuserDlg::OnOK()
{
	// The user probably intended to press enter for the setting change to take effect
	// Using killFocus event to update setting so just change focus
	SetFocus();
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CNGEuserDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPaintDC dc(this); // device context for painting
	
		CRect rect;
		GetDlgItem(IDC_MOSAIC_IMAGE)->GetWindowRect(&rect);
		ScreenToClient(&rect);
	
		DrawMosaic(dc, rect);
		
		// Draw fullscreen on the second monitor
		CMonitor monitor;
		CMonitors monitors;

		for (int i = 0; i < monitors.GetCount(); i++)
		{
			
#ifdef DEBUG_MONITOR_JAVQUI
			if (i == 1) { continue; } // javqui, temporal, just to skip teh debug monitor
#endif
			monitor = monitors.GetMonitor(i);
			if (!monitor.IsPrimaryMonitor() )   
			{
				CRect rect;
				monitor.GetMonitorRect(&rect);
				CMonitorDC dc(&monitor);

				DrawMosaic(dc, rect);
			}
		}

		CDialog::OnPaint();
	}
}


void CNGEuserDlg::OnSize(UINT nType, int cx, int cy)
{
	CWnd* pMosaicWnd = GetDlgItem(IDC_MOSAIC_IMAGE);

	if (pMosaicWnd)
	{
		// The following settings assume 96 DPI
		int SETTINGS_WIDTH_96DPI  = 500;
		int SETTINGS_HEIGHT_96DPI = 1000;
		int SETTINGS_BORDER_96DPI = 20;

		CDC *screen = GetDC();
		int dpiX = GetDeviceCaps(screen->m_hDC, LOGPIXELSX);
		ReleaseDC(screen);

		int settingsWidth = MulDiv(SETTINGS_WIDTH_96DPI, dpiX, 96);
		int settingsHeight = MulDiv(SETTINGS_HEIGHT_96DPI, dpiX, 96);
		int border = MulDiv(SETTINGS_BORDER_96DPI, dpiX, 96);

		int leftTab = cx - (settingsWidth + border);
		CRect rect;

		// Move the image
		GetDlgItem(IDC_MOSAIC_IMAGE)->GetWindowRect(&rect);
		ScreenToClient(&rect);
		// The width of the mosaic needs to be a multiple of 4
		int height = cy - 2*rect.top;
		int width = leftTab - border;
		width -= (width % 4);
		//GetDlgItem(IDC_MOSAIC_IMAGE)->SetWindowPos(NULL, rect.left, rect.top, width, height, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
		GetDlgItem(IDC_MOSAIC_IMAGE)->MoveWindow(rect.left, rect.top, width, height);
		int nextTop = rect.top;
/*
		// Move the logo
		GetDlgItem(IDC_LOGO)->GetWindowRect(&rect);
		ScreenToClient(&rect);
		GetDlgItem(IDC_LOGO)->MoveWindow(leftTab + 10, nextTop, rect.Width(), rect.Height());
		nextTop += rect.Height() + 40;

		// Move the screen selection buttons
		m_dScreenSelectDlg.GetWindowRect(&rect);
		ScreenToClient(&rect);
		m_dScreenSelectDlg.MoveWindow(leftTab + 10, nextTop, rect.Width(), rect.Height());
		nextTop += rect.Height() + 20;
*/
		// Move the setting scren
		_rSettingsRect = CRect(leftTab + 15, nextTop, settingsWidth, settingsHeight);
		ShowWindowNumber(_nSettingsScreen);

	}

	//CDialog::OnSize(nType, cx, cy);
}

void CNGEuserDlg::DrawMosaic(CDC &dc, CRect &rect)
{
	int height = _cvMosaicImg.rows;
	int width = _cvMosaicImg.cols;
	if ((height > 0) && (width > 0))
	{
		double scaleH = (double)rect.Height() / height;
		double scaleW = (double)rect.Width() / width;
		double scale = min(scaleH, scaleW);
		Mat cvScaledImg;
		resize(_cvMosaicImg, cvScaledImg, Size(), scale, scale);
		height = cvScaledImg.rows;
		width = cvScaledImg.cols;

		Mat cvFullImage = Mat::zeros(rect.Height(), rect.Width(), CV_8UC3);
		Rect roiCenter = Rect((rect.Width() - width) / 2, (rect.Height() - height) / 2, width, height);
		Mat cvCenter = cvFullImage(roiCenter);
		cvScaledImg.copyTo(cvCenter);

		CompStatus systemStatus = theApp.GetSystemManager()->GetSystemStatus().GetOverallSystemStatus();
		CompStatus calStatus = theApp.GetSystemManager()->GetSelfCalibration()->GetLastSelfCalibration().m_OverallCalStatus;

		bool bCalibrationRequired = true;
		CProcessorBoxSettings *pProcessorBoxSettings = theApp.GetSystemManager()->GetProcessorBoxSettings();
		if (pProcessorBoxSettings != NULL)
		{
			bCalibrationRequired = pProcessorBoxSettings->IsScopeCalibrationRequired();
		}

		DisplayMode currentMode = theApp.GetSystemManager()->GetDisplayMode();
		string msg[3];
		for (int i = 0; i < 3; i++)
		{
			msg[i] = "";
		}
		Scalar color = 0;

		switch (currentMode)
		{
		case eLive:
			if (systemStatus == eCalibrating)
			{
				color = Scalar(0, 255, 255);
				msg[0] = "CALIBRATING";
				msg[1] = "SYSTEM";
			}
			else if (systemStatus != eOK)
			{
				color = Scalar(0, 0, 255);
				msg[0] = "DEGRADED";
				msg[1] = "IMAGE";

				if (systemStatus == eFatal)
				{
					msg[2] = "UNABLE TO RECOVER";
				}
				else
				{
					msg[2] = "RECOVERING";
				}
			}
			else if ((calStatus != eOK) && bCalibrationRequired)
			{
				color = Scalar(0, 255, 255);
				msg[0] = "UNCALIBRATED";
				msg[1] = "IMAGE";
			}
			else
			{
				color = Scalar(0, 255, 0);
				msg[0] = "LIVE";
				msg[1] = "IMAGE";
			}
			
			break;
		case eFrozen:
			color = Scalar(255, 0, 0);
			msg[0] = "STREAM";
			msg[1] = "FROZEN";
			break;
		case eRecording:
			color = Scalar(0, 0, 255);
			msg[0] = "RECORDING";
			break;
		case ePlayback:
			color = Scalar(255, 0, 0);
			msg[0] = "VIDEO";
			msg[1] = "PLAYBACK";
			break;
		case eDisplayImage:
			color = Scalar(255, 0, 0);
			msg[0] = "IMAGE";
			msg[1] = "DISPLAY";
			break;
		}

		int font = FONT_HERSHEY_PLAIN;
		int thickness = 2;
		int baseline = 0;

		CDC *screen = GetDC();
		int dpiX = GetDeviceCaps(screen->m_hDC, LOGPIXELSX);
		ReleaseDC(screen);

		double FONT_SCALE_96DPI = 1.5;
		double fontScale = dpiX * FONT_SCALE_96DPI / 96;
		
		Size textSize0 = getTextSize(msg[0], font, fontScale, thickness, &baseline);
		Size textSize1 = getTextSize(msg[1], font, fontScale, thickness, &baseline);
		Size textSize2 = getTextSize(msg[2], font, fontScale, thickness, &baseline);
		int maxWidth = max(max(textSize0.width, textSize1.width), textSize2.width);
		Point textCenter = Point(rect.Width() - maxWidth / 2 - 40, 40);
		int lineSeparation = 10;
		int rectBorder = 10;
		Rect backgroundRect = Rect(textCenter.x - maxWidth / 2 - rectBorder, textCenter.y - textSize0.height - rectBorder,
			maxWidth + 2 * rectBorder, 4 * textSize0.height + 2 * rectBorder + 2 * lineSeparation);
		cv::rectangle(cvFullImage, backgroundRect, cv::Scalar(0, 0, 0), -1);

		cv::putText(cvFullImage, msg[0], Point(textCenter.x - textSize0.width / 2, textCenter.y), 
			font, fontScale, color, thickness);
		cv::putText(cvFullImage, msg[1], Point(textCenter.x - textSize1.width / 2, textCenter.y + textSize1.height + lineSeparation),
			font, fontScale, color, thickness);
		cv::putText(cvFullImage, msg[2], Point(textCenter.x - textSize2.width / 2, textCenter.y + textSize1.height + lineSeparation + 2 * textSize2.height + lineSeparation),
			font, fontScale, color, thickness);

		if (theApp.GetSystemManager()->IsSelectBandImagingEnabled())
		{
			string msg2[3];
			Scalar color2 = Scalar(0, 255, 0);
			msg2[0] = "SELECT";
			msg2[1] = "BAND";
			msg2[2] = "IMAGING";

			textCenter = Point(100, 40);
			textSize0 = getTextSize(msg2[0], font, fontScale, thickness, &baseline);
			textSize1 = getTextSize(msg2[1], font, fontScale, thickness, &baseline);
			Size textSize2 = getTextSize(msg2[2], font, fontScale, thickness, &baseline);

			maxWidth = max(textSize0.width, textSize1.width);
			maxWidth = max(textSize2.width, maxWidth);
			backgroundRect = Rect(textCenter.x - maxWidth / 2 - rectBorder, textCenter.y - textSize0.height - rectBorder,
				maxWidth + 2 * rectBorder, (textSize0.height + rectBorder) * 3 + lineSeparation);
			cv::rectangle(cvFullImage, backgroundRect, cv::Scalar(0, 0, 0), -1);

			cv::putText(cvFullImage, msg2[0], Point(textCenter.x - textSize0.width / 2, textCenter.y),
				font, fontScale, color, thickness);
			cv::putText(cvFullImage, msg2[1], Point(textCenter.x - textSize1.width / 2, textCenter.y + textSize1.height + lineSeparation),
				font, fontScale, color, thickness);
			cv::putText(cvFullImage, msg2[2], Point(textCenter.x - textSize2.width / 2, textCenter.y + textSize1.height + lineSeparation + textSize2.height + lineSeparation),
				font, fontScale, color, thickness);
		}

		uchar buffer[sizeof(BITMAPINFOHEADER) + 1024];
		BITMAPINFO* bmi = (BITMAPINFO*)buffer;
		FillBitmapInfo(bmi, rect.Width(), rect.Height(), 8 * cvFullImage.channels(), 0);

		SetDIBitsToDevice(dc.GetSafeHdc(), rect.left, rect.top, rect.Width(),
			rect.Height(), 0, 0, 0, rect.Height(), cvFullImage.data, bmi,
			DIB_RGB_COLORS);
	}
}

void CNGEuserDlg::FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin)
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


LRESULT CNGEuserDlg::OnSelectScreen(WPARAM control, LPARAM value)
{
	ShowWindowNumber(control);

	return 0;
}

void CNGEuserDlg::CaptureImage(string file)
{
	m_sCaptureFile = file;
}

void CNGEuserDlg::DisplayImage(string file)
{
	m_sDisplayFile = file;
}

void CNGEuserDlg::CaptureVideo(string file)
{
	m_sVideoFile = file;
}

void CNGEuserDlg::StopCapture()
{
	m_sVideoFile = "";
}

void CNGEuserDlg::PlaybackFile(string file)
{
	m_sPlaybackFile = file;
}

UINT CNGEuserDlg::StitchLoop()
{
	Mat mosaic;
	VideoWriter videoWriter;
	VideoCapture videoCapture;

	while (!_bExiting)
	{
		// Make any required mode changes
		DisplayMode currentDisplayMode = theApp.GetSystemManager()->GetDisplayMode();
		DisplayMode newDisplayMode = theApp.GetSystemManager()->GetNewDisplayMode();
		if (newDisplayMode != currentDisplayMode)
		{
			currentDisplayMode = newDisplayMode;
			theApp.GetSystemManager()->SetDisplayMode(currentDisplayMode);
			m_dProcessingDlg.PostMessage(WM_UPDATE_MODE_MSG, NULL);
			m_dMainDlg.PostMessage(WM_UPDATE_MODE_MSG, NULL);
		}

		try
		{
			if (m_sDisplayFile.size() > 0)
			{
				// Display a new image
				_cvMosaicImg = imread(m_sDisplayFile);
				m_sDisplayFile = "";
				theApp.GetSystemManager()->SetNewDisplayMode(eDisplayImage);
			}
			else if (m_sPlaybackFile.size() > 0)
			{
				// Playback a new video
				videoCapture.open(m_sPlaybackFile);
				if (videoCapture.isOpened())
				{
					theApp.GetSystemManager()->SetNewDisplayMode(ePlayback);
				}
				m_sPlaybackFile = "";
			}
			else if (currentDisplayMode == ePlayback)
			{
				int numFrames = videoCapture.get(CV_CAP_PROP_FRAME_COUNT);
				int curFrame = videoCapture.get(CV_CAP_PROP_POS_FRAMES);
				if (curFrame == (numFrames - 1))
				{
					videoCapture.set(CV_CAP_PROP_POS_FRAMES, 0);
				}
				videoCapture >> _cvMosaicImg;

				Sleep(33);
			}
			else if ((currentDisplayMode == eDisplayImage) || (currentDisplayMode == eFrozen))
			{
				// Prevent constant redrawing
				Sleep(100);
			}
			else
			{ 
				HANDLE hEvent = theApp.GetSystemManager()->GetCameraData(eFront)->GetUpdateEvent();
				if (hEvent != NULL)
				{
					WaitForSingleObject(hEvent, 200);
					ResetEvent(hEvent);
				}

				// Create the mosaic from the cameras
				MosaicData *pMosaicData = theApp.GetSystemManager()->GetMosaicData();
				pMosaicData->CreateMosaic(_cvMosaicImg);

				if (m_sCaptureFile.size() > 0)
				{
					imwrite(m_sCaptureFile, _cvMosaicImg);
					m_sCaptureFile = "";
				}

				if (m_sVideoFile.size() > 0)
				{
					if (!videoWriter.isOpened())
					{
						// Open a new recording stream
						videoWriter.open(m_sVideoFile, CV_FOURCC('X', 'V', 'I', 'D'), 30, _cvMosaicImg.size(), true);
						if (videoWriter.isOpened())
						{
							theApp.GetSystemManager()->SetNewDisplayMode(eRecording);
						}
						else
						{
							theApp.ShowMessageBox(_T("Video compressor is not installed properly"), _T("Error"), MB_OK | MB_TOPMOST);
							m_sVideoFile = "";
						}
					}
					if (videoWriter.isOpened())
					{
						
						videoWriter.write(_cvMosaicImg);
					}
				}
				else if (videoWriter.isOpened())
				{
					// Close the recording stream
					videoWriter.release();
					theApp.GetSystemManager()->SetNewDisplayMode(eLive);
				}
			}
				
			RedrawDisplay();
		}
		catch (cv::Exception ex)
		{
			LOG4CXX_ERROR(logger, ex.msg);
		}
		catch (...)
		{
			LOG4CXX_ERROR(logger, "StitchLoop exception creating mosaic");
		}
	}

	LOG4CXX_INFO(logger, "StitchLoop is complete");

	return 0;
}

void CNGEuserDlg::RedrawDisplay()
{
	CRect rect;
	GetDlgItem(IDC_MOSAIC_IMAGE)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	InvalidateRect(rect, FALSE);
}

void CNGEuserDlg::UpdateTitle()
{
	SetWindowText(_T(WINDOW_NAME));
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CNGEuserDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CNGEuserDlg::AddTabs(bool advanced)
{
	for (int i = 0; i < _vDlgPointers.size(); i++)
	{
		_vDlgPointers[i]->ShowWindow(SW_HIDE);
	}
	_vDlgPointers.clear();
//	_vDlgPointers.push_back(&m_dMainDlg);
	_vDlgPointers.push_back(&m_dCefBrowserDlg);

	if (advanced)
	{
		m_dScreenSelectDlg.ShowWindow(SW_SHOW);
		AddAdvancedTabs();
		ShowWindowNumber(1);
	}
	else
	{
		m_dScreenSelectDlg.ShowWindow(SW_HIDE);
		ShowWindowNumber(0);
	}
}

void CNGEuserDlg::AddAdvancedTabs()
{
	_vDlgPointers.push_back(&m_dProcessingDlg);
	_vDlgPointers.push_back(&m_dLightingDlg);
	_vDlgPointers.push_back(&m_dHandsetDlg);
	_vDlgPointers.push_back(&m_dEnhancementDlg);
	_vDlgPointers.push_back(&m_dSelfCalibrationDlg);
	_vDlgPointers.push_back(&m_dSystemManagerDlg);
	_vDlgPointers.push_back(&m_dCameraDlg);
	_vDlgPointers.push_back(&m_dMicroControlDlg);
	_vDlgPointers.push_back(&m_dSignalOutputDlg);

#ifndef USE_USB_CAMERA
	tabItem.pszText = _T("  FPGA   ");
	m_cTab.InsertItem(_vDlgPointers.size(), &tabItem);
	_vDlgPointers.push_back(&m_dFPGADlg);
#endif
}

void CNGEuserDlg::ShowWindowNumber(int number)
{
	int windowCount = _vDlgPointers.size();

	// Validate the parameter
	if ((number >= 0) && (number < windowCount))
	{
		// Hide every window except for the chosen one
		for (int count = 0; count < windowCount; count++)
		{
			if (count != number)
			{
				_vDlgPointers[count]->ShowWindow(SW_HIDE);
			}
			else if (count == number)
			{
				// Show the chosen window and set it's location
				_vDlgPointers[count]->SetWindowPos(&wndTop,
					_rSettingsRect.left, _rSettingsRect.top, 
					500, _rSettingsRect.Height(), 0);
				_vDlgPointers[count]->ShowWindow(SW_SHOW);
			}
		}
		_nSettingsScreen = number;
		m_dScreenSelectDlg.SetScreenSelect(number);
	}
}

void CNGEuserDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);

	// When the dialog is shown, display the first window
	if (bShow)
	{
		ShowWindowNumber(0);
	}
}


void CNGEuserDlg::ReloadControls(string folder)
{
	m_dHandsetDlg.ReloadControls(folder);
}

void CNGEuserDlg::SaveControls(string folder)
{
	m_dHandsetDlg.SaveControls(folder);
}
