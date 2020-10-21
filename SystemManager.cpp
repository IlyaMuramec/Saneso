#include "stdafx.h"
#include "SystemManager.h"
#include "MicroController.h"
#include "NGEuserDlg.h"
#include "NGEuserApp.h"
#include "ProcessorBoxSettings.h"
#include "SplashScreen.h"
#include "ScopeSettings.h"
#include "ScopeStatus.h"
#include "SelfCalibration.h"
#include "SignalOutput.h"
#include "Timers.h"
#include <fstream>

log4cxx::LoggerPtr  CSystemManager::logger(log4cxx::Logger::getLogger("SystemManager"));

CSystemManager::CSystemManager()
{
	m_pMainDlg = NULL;
	m_pSystemManagerDlg = NULL;
	m_bModifiedFlag = false;
	m_bExiting = false;
	m_hMonitorThread = NULL;
	m_hMicroControllerReconnectThread = NULL;
	m_hScopeReconnectThread = NULL;
	m_pScopeEEPromSettings = NULL;
	m_pScopeCurrentSettings = NULL;
	m_pProcessorEEPromSettings = NULL;
	m_MicroControllerState = eNoPower;

	m_pScopeStatus = new CScopeStatus();
	m_pMicroController = new CMicroController(m_pScopeStatus);
	//m_pMicroController->AddConnectCallback(MicroControllerConnectedCallback, this);

	m_pSelfCalibration = new CSelfCalibration(this);
	m_pSignalOutput = new CSignalOutput();
}

CSystemManager::~CSystemManager()
{
	if (m_pLightControl)
		delete m_pLightControl;

	if (m_pMicroController)
		delete m_pMicroController;

	if (m_pMosaicData)
		delete m_pMosaicData;

	if (m_pEnhancementControl)
		delete m_pEnhancementControl;

	if (m_pSignalOutput)
		delete m_pSignalOutput;
}

void CSystemManager::Create(CNGEuserDlg *pDlg)
{
	LOG4CXX_INFO(logger, "Create() begin");
	m_pMainDlg = pDlg;
	m_pLightControl = NULL;
	CSplashThread *pSplash = theApp.GetSplashThread();
	
	// Process with new classes
	m_bImageDebuggingEnabled = FALSE;
	m_eNewDisplayMode = eLive;
	m_eCurrentDisplayMode = eLive;

	string configFolder = DEFAULT_CONFIG;
	string outputFolder = DEBUG_FOLDER;

	pSplash->SetText("Loading Enhancement Settings");

	m_pEnhancementControl = new CEnhancementControl();
	m_pEnhancementControl->Load(configFolder);

	CreateDirectoryA(outputFolder.c_str(), NULL);

	string mosaicFolder = outputFolder + "\\Mosaic";
	CreateDirectoryA(mosaicFolder.c_str(), NULL);

	m_pMosaicData = new MosaicData(&m_ProcTimes);
	m_pMosaicData->EnableApplyMask(FALSE);
	m_pMosaicData->EnableMarkerDislay(TRUE);
	m_pMosaicData->SetImageStitchingMode(eNormal);
	if (m_bImageDebuggingEnabled)
	{
		m_pMosaicData->SetOutputFolder(mosaicFolder);
	}

	// Create the cameras
	LOG4CXX_INFO(logger, "Creating Cameras");
	for (int i = 0; i < 5; i++)
	{
		CString camName(CameraNames[i].c_str());
		CString msg = _T("Creating Camera - ") + camName;
		pSplash->SetText(msg);

		CameraType camType = (CameraType)i;

		// Create the camera
		CameraData *pCamera = new CameraData(camType, m_pMosaicData, m_pEnhancementControl, &m_ProcTimes);
		if (m_bImageDebuggingEnabled)
		{
			pCamera->SetOutputFolder(outputFolder);
		}
		pCamera->Load(DEFAULT_CONFIG);
		pCamera->LoadRawImage(configFolder + "\\RawImage_" + CameraNames[i] + ".bmp");
		m_vCameras.push_back(pCamera);
	}


	// Setup the light control
	LOG4CXX_INFO(logger, "Creating LightControl");
	pSplash->SetText("Loading Light Values");

	m_pLightControl = new CLightControl(m_pMicroController);
	m_pLightControl->ReloadSettings(configFolder);


#ifdef USE_USB_CAMERA

	LOG4CXX_INFO(logger, "Creating USB Reader");
	int ret = m_USBReader.Initialize(m_vCameras, outputFolder, configFolder);

#else
	// Start the Pentek Data Capture
	int ret = m_PentekReader.Initialize(m_vCameras, outputFolder);
	if (ret != 0)
	{
		string msg = "Error initializing Pentek: " + m_PentekReader.GetErrorMessage(ret);
		LOG4CXX_ERROR(logger, msg);

		msg += "\n\nUse simulation data?";
		int ret = theApp.ShowMessageBox(msg.c_str(), "Error", MB_YESNO | MB_TOPMOST);
		if (ret == IDYES)
		{
			m_PentekReader.LoadSimulationData();
		}
	}
#endif
	
	LOG4CXX_INFO(logger, "Connecting to  MicroController");

#ifndef IGNORE_MICROCONTROLLER
	// Connect the Micro Controller
	StartMicroControllerReconnectThread();
#else
	m_MicroControllerState = eConnected;

	pSplash->SetText("Micro Controller is SIMULATED!");
	//m_pMicroController->SimulateControlMessages();
	Sleep(1000);
#endif

	// Connect the Signal Output to COM1
	m_pSignalOutput->Connect(1);

	// Start the image capture if the micro controller is connected
	if (m_MicroControllerState == eConnected)
	{
		Start(false);
	}
	else
	{
		StartMonitor();
	}

	LOG4CXX_INFO(logger, "Create() complete");
}
	
void CSystemManager::Start(bool bRecovery)
{
	LOG4CXX_INFO(logger, "Start() Recovery flag = " << bRecovery);

	if (!bRecovery)
	{
		// Reset the calibration
		m_pSelfCalibration->ResetCalibration();
	}

	CSplashThread *pSplash = theApp.GetSplashThread();
	pSplash->SetText("Starting Scope");
	pSplash->ShowSplash();

#ifndef IGNORE_MICROCONTROLLER

	// Send the reset command to the USB chips
	LOG4CXX_INFO(logger, "Resetting all USB chips")
	m_USBReader.ResetAllUSBChips();

	// Cycle power to the camera chips
	LOG4CXX_INFO(logger, "Toggling camera reset line");
	m_pMicroController->ToggleCameraResetLine(bRecovery);

#endif

	if (!m_pScopeStatus->IsScopeConnected())
	{
		LOG4CXX_ERROR(logger, "Cannot start disconnected scope");
		StartMonitor();
		return;
	}

	if (!bRecovery)
	{
		pSplash->SetText("Loading settings from EEPROM");

		// Create the current scope settings object
		vector<CCameraSettings*> cameraSettings;
		for (int i = 0; i < 5; i++)
		{
			cameraSettings.push_back(m_vCameras[i]->GetCameraSettings());
		}
		m_pScopeCurrentSettings = new CScopeSettings("Unassigned", cameraSettings, m_pLightControl->GetLightCalSettings());

		// Load the settings from the EEPROM 
		ReadEEPromSettings();

#if 0//def IGNORE_MICROCONTROLLER
		std::ifstream file("C:\\NGE\\Scope Read.txt");
		std::string str((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		file.close();

		if (m_pScopeEEPromSettings == NULL)
			delete m_pScopeEEPromSettings;

		m_pScopeEEPromSettings = new CScopeSettings(str);
#endif
	}

	if (m_pScopeEEPromSettings->GetNumCameras() == 5)
	{
		LOG4CXX_INFO(logger, "Found camera settings in EEPROM, so loading them");
		m_pScopeCurrentSettings->UpdateFromEEPromString(m_pScopeEEPromSettings->GetEEPromString());
		for (int i = 0; i < 5; i++)
		{
			m_vCameras[i]->RefreshCameraSettings();
		}
	}
	else
	{
		string msg = "Invalid camera data found in EEPROM.\nUsing Default config values.";
		LOG4CXX_ERROR(logger, msg);
		theApp.ShowMessageBox(msg.c_str(), "Error", MB_OK | MB_TOPMOST);
	}

	pSplash->SetText("Downloading settings to Micro Controller");

	if (m_pLightControl)
	{
		m_pLightControl->MicroControllerConnected();
	}
		
	for (int i = 0; i < m_vCameras.size(); i++)
	{
		CString camName(CameraNames[i].c_str());
		CString msg = _T("Starting Camera - ") + camName;
		pSplash->SetText(msg);
		m_vCameras[i]->StartWarpThread();
		m_vCameras[i]->MicroControllerConnected(m_pMicroController);
	}

	m_USBReader.StartCapture();
	
	StartMonitor();

	pSplash->HideSplash();
}

void CSystemManager::StartMonitor()
{
	LOG4CXX_INFO(logger, "Starting Monitor thread");

	DWORD dwThreadID;
	m_hMonitorThread = CreateThread(
		NULL,         // default security attributes
		0,            // default stack size
		(LPTHREAD_START_ROUTINE)&CSystemManager::ThreadProc,
		(LPVOID)this,         // no thread function arguments
		0,            // default creation flags
		&dwThreadID); // receive thread identifier
}

void CSystemManager::StopMonitor()
{
	LOG4CXX_INFO(logger, "Stopping Monitor thread");

	m_bExiting = true;
	WaitForSingleObject(m_hMonitorThread, INFINITE);
	m_hMonitorThread = NULL;
	m_bExiting = false;

	LOG4CXX_INFO(logger, "Stopping Monitor thread complete");
}

void CSystemManager::ResetUSBChips(CSplashThread *pSplash)
{
	pSplash->SetText("Resetting USB Chips");
	pSplash->ShowSplash();

	StopMonitor();
	StopCapture();

	// USB chips will be reset during Start()
	Start(true);

	LOG4CXX_INFO(logger, "ResetUSBChips() complete");

	pSplash->HideSplash();
}


//void CSystemManager::MicroControllerConnectedCallback(void* pObject)
//{
//	CSystemManager *pSelf = (CSystemManager*)pObject;
//	pSelf->MicroControllerConnected();
//}
//
//void CSystemManager::MicroControllerConnected()
//{
//	if (m_pLightControl)
//		m_pLightControl->MicroControllerConnected();
//
//	for (int i = 0; i < m_vCameras.size(); i++)
//	{
//		m_vCameras[i]->MicroControllerConnected(m_pMicroController);
//	}
//}

void CSystemManager::Close()
{
	StopMonitor();

	m_pLightControl->StopAuto();

#ifdef USE_USB_CAMERA
	m_USBReader.StopCapture();
#else
	m_PentekReader.Close();
#endif

	for (int i = 0; i < m_vCameras.size(); i++)
	{
		delete m_vCameras[i];
	}
	m_vCameras.clear();
}

void CSystemManager::StopCapture()
{
	LOG4CXX_INFO(logger, "Stopping capture");

	// Update the display to show latest status
	theApp.GetMainDlg()->RedrawDisplay();

	m_USBReader.StopCapture();
	
	// Stop the cameras
	for (int i = 0; i < m_vCameras.size(); i++)
	{
		m_vCameras[i]->StopWarpThread();
	}
}

void CSystemManager::SetImageStichingMode(ImageStitchingMode newMode)
{
	m_pMosaicData->SetImageStitchingMode(newMode);

	for (int i = 0; i < m_vCameras.size(); i++)
	{
		m_vCameras[i]->SetImageStitchingMode(newMode);
	}
}

void CSystemManager::NextImageStitchingMode()
{
	int mode = (int)m_pMosaicData->GetImageStitchingMode();
	mode++;
	if (mode == (int)eLastMode)
	{
		mode = 0;
	}
	ImageStitchingMode newMode = (ImageStitchingMode)mode;
	m_pMosaicData->SetImageStitchingMode(newMode);

	for (int i = 0; i < m_vCameras.size(); i++)
	{
		m_vCameras[i]->SetImageStitchingMode(newMode);
	}
}

void CSystemManager::EnableSelectBandImaging(bool enable)
{
	m_bSelectBandImaging = enable;
	m_pLightControl->EnableSelectBandImaging(m_bSelectBandImaging);
}

void CSystemManager::EnableApplyMask(bool enable)
{
	m_pMosaicData->EnableApplyMask(enable);
}

BOOL CSystemManager::IsApplyMaskEnabled()
{
	return m_pMosaicData->IsApplyMaskEnabled();
}

void CSystemManager::EnableMarkerDislay(bool enable)
{
	m_pMosaicData->EnableMarkerDislay(enable);
}

BOOL CSystemManager::IsMarkerDisplayEnabled()
{
	return m_pMosaicData->IsMarkerDisplayEnabled();
}


void CSystemManager::SetModifiedFlag(BOOL modified)
{
	m_bModifiedFlag = modified;
	if (m_pMainDlg)
		m_pMainDlg->UpdateTitle();
}

void CSystemManager::SaveConfig(CString file)
{
	try
	{
		std::string folder = CStringA(file.Left(file.ReverseFind(_T('\\'))));

		//Make a backup of Default
		if (folder == DEFAULT_CONFIG)
		{
			std::string dateTime = GetDateTime();
			std::string backup = folder + "_" + dateTime;


			MoveFileExA(folder.c_str(), backup.c_str(), MOVEFILE_WRITE_THROUGH);
			std::string msg = "Default backed up to: " + backup;
			theApp.ShowMessageBox(msg.c_str(), "Info", MB_OK | MB_TOPMOST);
		}
		LOG4CXX_INFO(logger, "Save Config to + " + folder);

		CreateDirectoryA(folder.c_str(), NULL);

		m_pEnhancementControl->Save(folder);
		m_ProgramSettings.Save(folder);
		m_pLightControl->SaveSettings(folder);

		for (int i = 0; i < m_vCameras.size(); i++)
		{
			m_vCameras[i]->Save(folder);
		}

		if (m_pMainDlg)
		{
			m_pMainDlg->SaveControls(folder);
		}
	}
	catch (...)
	{
		LOG4CXX_ERROR(logger, "An exception occurred saving file!");
	}
}

void CSystemManager::LoadConfig(CString file)
{
	try
	{
		CFileStatus status;
		if (!CFile::GetStatus(file, status))
		{
			LOG4CXX_ERROR(logger, "File doesn't exist!");
			return;
		}

		std::string folder = CStringA(file.Left(file.ReverseFind(_T('\\'))));
		LOG4CXX_INFO(logger, "Load Config from " + folder);

		LoadSettings(folder);
	}
	catch (...)
	{
		LOG4CXX_ERROR(logger, "An exception occurred loading file!");
	}
}

void CSystemManager::CaptureImageFile()
{
	m_pSignalOutput->SendTrigger();

	string dateTime = GetDateTime();

	CStringA imageFolder = theApp.GetImageCaptureFolder();
	std::string file = imageFolder + "\\";;
	file += dateTime;
	file += ".png";

	CreateDirectoryA(imageFolder, NULL);

	m_pMainDlg->CaptureImage(file);
}

void CSystemManager::CaptureVideoFile()
{
	string dateTime = GetDateTime();

	CStringA videoFolder = theApp.GetVideoCaptureFolder();
	std::string file = videoFolder + "\\";
	file += dateTime;
	file += ".avi";

	CreateDirectoryA(videoFolder, NULL);

	m_pMainDlg->CaptureVideo(file);
}

string CSystemManager::GetDateTime()
{
	//Create unique timestamp
	time_t rawtime;
	struct tm * timeinfo;
	char buf[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buf, 80, "%Y%m%d_%H%M%S", timeinfo);

	return buf;
}

CameraData *CSystemManager::GetCameraData(CameraType type)
{
	CameraData *pCamera = NULL;

	int nIndex = (int)type;
	if ((nIndex >= 0) && (nIndex < m_vCameras.size()))
	{
		pCamera = m_vCameras[nIndex];
	}

	return pCamera;
}

void CSystemManager::UpdateAllCameraModels()
{
	for (int i = 0; i < m_vCameras.size(); i++)
	{
		m_vCameras[i]->UpdateThinPlateSpline();
	}
}



void CSystemManager::LoadSettings(string folder)
{
	m_eCurrentDisplayMode = m_eNewDisplayMode = eFrozen;

	StopMonitor();
	StopCapture();

	m_ProgramSettings.Load(folder);
	m_pLightControl->ReloadSettings(folder);
	m_pEnhancementControl->Load(folder);
	m_USBReader.SetSimImageFolder(folder);

	EnableSelectBandImaging(false);

	// Start the cameras
	for (int i = 0; i < m_vCameras.size(); i++)
	{
		m_vCameras[i]->Load(folder);
		m_vCameras[i]->LoadRawImage(folder + "\\RawImage_" + CameraNames[i] + ".bmp");
		m_vCameras[i]->StartWarpThread();
	}

	if (m_pMainDlg)
	{
		m_pMainDlg->ReloadControls(folder);
	}

	Start(false);

	m_eCurrentDisplayMode = m_eNewDisplayMode = eLive;
}

void CSystemManager::LoadDefaultSettings()
{
	m_bStreamUpdateEnabled = TRUE;
	m_pMosaicData->EnableApplyMask(FALSE);
	m_pMosaicData->EnableMarkerDislay(TRUE);
	m_pMosaicData->SetImageStitchingMode(eNormal);

	LoadSettings(DEFAULT_CONFIG);
}


void CSystemManager::DumpProcTimes(string folder)
{
	m_ProcTimes.SaveTimes(folder + "\\ProcTimes.txt");

}

CompStatus CSystemManager::GetCameraStatus(double fps)
{
	CompStatus status = eUnknown;

	if (fps > 20)
		status = eOK;
	else if (fps > 5)
		status = eMarginal;
	else
		status = eError;

	return status;
}

CompStatus CSystemManager::GetMosaicStatus(double fps)
{
	CompStatus status = eUnknown;

	if (fps > 20)
		status = eOK;
	else if (fps > 5)
		status = eMarginal;
	else
		status = eError;

	return status;
}

CompStatus CSystemManager::GetControllerStatus(double heartbeat)
{
	CompStatus status = eUnknown;

	if (heartbeat >= 0.8)
		status = eOK;
	else if (heartbeat >= 0.2)
		status = eMarginal;
	else
		status = eError;

	return status;
}

void CSystemManager::CalibrateCamerasWhite(unsigned int mask)
{
	// For each camera
	for (int i = 0; i < 5; i++)
	{
		if (mask & (1 << i))
		{
			// Do white balance
			// Ideal pink color for colon model
			double calBlueComp = 0.54;
			double calGreenComp = 1.0;
			double calRedComp = 1.71;
			m_vCameras[i]->CalibrateWhiteBalance(calRedComp, calGreenComp, calBlueComp);
		}
	}
}

void CSystemManager::CalibrateCamerasFPN(unsigned int mask, CSplashThread* pSplash)
{
	pSplash->SetText("Turning off lights");
	pSplash->ShowSplash();

	// Turn off all lights
	bool bFrontAuto = m_pLightControl->GetFrontLightsAuto();
	m_pLightControl->SetFrontLightsAuto(false, true);
	m_pLightControl->SetFrontLightsOn(false, true);

	bool bSideAuto = m_pLightControl->GetSideLightsAuto();
	m_pLightControl->SetSideLightsAuto(false, true);
	m_pLightControl->SetSideLightsOn(false, true);

	// For each camera
	time_t start, now;
	for (int i = 0; i < 5; i++)
	{
		if (mask & (1 << i))
		{
			CString camName(CameraNames[i].c_str());
			CString msg = _T("Calibrating FPN Camera - ") + camName;
			pSplash->SetText(msg);

			// Calibrate FPN
			m_vCameras[i]->CalibrateFPN();
			time(&start);
			while (m_vCameras[i]->IsFPNCalActive())
			{
				Sleep(500);
				time(&now);
				double seconds = difftime(now, start);
				if (seconds > 10)
				{
					pSplash->SetText("Timeout waiting for FPN cal");
					break;
				}
			}
		}
	}

	// Turn lights back on
	pSplash->SetText("Turning on lights");
	m_pLightControl->SetFrontLightsOn(true, true);
	m_pLightControl->SetFrontLightsAuto(bFrontAuto, true);

	m_pLightControl->SetSideLightsOn(true, true);
	m_pLightControl->SetSideLightsAuto(bSideAuto, true);

	pSplash->HideSplash();
}

void CSystemManager::ResetCameras(unsigned int mask)
{
#ifdef USE_USB_CAMERA
	m_USBReader.ResetCameras(mask);
#endif

}

void CSystemManager::InitializeCameras(unsigned int mask)
{
#ifdef USE_USB_CAMERA
	for (int i = 0; i < 5; i++)
	{
		if (mask & (1 << i))
		{
			m_vCameras[i]->MicroControllerConnected(m_pMicroController);
			LOG4CXX_INFO(logger, "Starting camera - " << m_vCameras[i]->GetName());
		}
	}
#endif

}

UINT CSystemManager::MonitorLoop()
{
	CompStatus prevSystemStatus = CompStatus::eUnknown;

	while (!m_bExiting)
	{
		Sleep(500);

		m_SystemStatus.m_dFPSMosaic = m_pMosaicData->GetFrameRate();
		m_SystemStatus.m_MosaicStatus = GetMosaicStatus(m_SystemStatus.m_dFPSMosaic);

		m_SystemStatus.m_dFPSTopCamera = m_vCameras[eTop]->GetFrameRate();
		m_SystemStatus.m_dFPSLeftCamera = m_vCameras[eLeft]->GetFrameRate();
		m_SystemStatus.m_dFPSBottomCamera = m_vCameras[eBottom]->GetFrameRate();
		m_SystemStatus.m_dFPSRightCamera = m_vCameras[eRight]->GetFrameRate();
		m_SystemStatus.m_dFPSFrontCamera = m_vCameras[eFront]->GetFrameRate();

		m_SystemStatus.m_CamerasStatus[eFront] = m_vCameras[eFront]->IsCameraSimulated() ? eSimulated : GetCameraStatus(m_SystemStatus.m_dFPSFrontCamera);
		m_SystemStatus.m_CamerasStatus[eTop] = m_vCameras[eTop]->IsCameraSimulated() ? eSimulated : GetCameraStatus(m_SystemStatus.m_dFPSTopCamera);
		m_SystemStatus.m_CamerasStatus[eBottom] = m_vCameras[eBottom]->IsCameraSimulated() ? eSimulated : GetCameraStatus(m_SystemStatus.m_dFPSBottomCamera);
		m_SystemStatus.m_CamerasStatus[eLeft] = m_vCameras[eLeft]->IsCameraSimulated() ? eSimulated : GetCameraStatus(m_SystemStatus.m_dFPSLeftCamera);
		m_SystemStatus.m_CamerasStatus[eRight] = m_vCameras[eRight]->IsCameraSimulated() ? eSimulated : GetCameraStatus(m_SystemStatus.m_dFPSRightCamera);

		m_SystemStatus.m_ControllerVersion = m_pMicroController->GetVersion();
		m_SystemStatus.m_ControllerStatusString = m_pMicroController->GetStatus();
		m_SystemStatus.m_dControllerHeartBeat = m_pMicroController->GetHeartBeat();
		
		m_SystemStatus.m_USBStatus = m_USBReader.GetStatus();
		m_SystemStatus.m_SelfCalibrationStatus = m_pSelfCalibration->IsCalibrating() ? eCalibrating : eOK;

		// Using the scope status bit recovering to override the USB Status
		if (m_pScopeStatus->IsScopeRecovering())
		{
			m_SystemStatus.m_USBStatus = CompStatus::eError;
			m_USBReader.InhibitRecovery(true);
		}
		else
		{
			m_USBReader.InhibitRecovery(false);
		}
		
		if (m_MicroControllerState == eConnected)
		{
			m_SystemStatus.m_ControllerStatus = GetControllerStatus(m_SystemStatus.m_dControllerHeartBeat);
		}
		else
		{
			m_SystemStatus.m_ControllerStatus = eError;
		}

		bool bResetUSBDevices = false;

#ifndef IGNORE_MICROCONTROLLER

		if ((m_MicroControllerState == eConnected)
			|| (m_MicroControllerState == eNoScope))
		{
			if ((m_SystemStatus.m_dControllerHeartBeat == 0) 
				|| (m_pMicroController->GetConnectedComPort() < 0))
			{
				LOG4CXX_INFO(logger, "MonitorLoop: Lost MicroController communication");
				
				// Lost haeart beat, so assume that power was lost
				m_MicroControllerState = eNoPower;
				m_SystemStatus.m_ControllerStatus = eError;
				StopCapture();
			}
			else if (m_MicroControllerState == eConnected)
			{
				if (!m_pScopeStatus->IsScopeConnected())
				{
					LOG4CXX_INFO(logger, "MonitorLoop: Scope disconnected");

					m_MicroControllerState = eNoScope;
					m_SystemStatus.m_ControllerStatus = eError;
					StopCapture();
				}
				else if (!m_pScopeStatus->IsScopeCommOK())
				{
					LOG4CXX_ERROR(logger, "MonitorLoop: Scope communication error. Resetting system");
					bResetUSBDevices = true;
				}
				else if (m_SystemStatus.m_USBStatus == CompStatus::eFatal)
				{
					int count = m_USBReader.GetResetCount();
					if (count < 5)
					{
						LOG4CXX_ERROR(logger, "MonitorLoop: USB Reader status is eFatal and reset count is " << count << ". Resetting system");
						m_USBReader.IncrementResetCount();
						m_SystemStatus.m_USBStatus == CompStatus::eError;
						bResetUSBDevices = true;
					}
					else
					{
						LOG4CXX_ERROR(logger, "MonitorLoop: USB Reader status is eFatal and reset count " << count << " exceeds limit");
					}
				}
			}
		}

		if (m_MicroControllerState == eNoScope)
		{
			// Start thred to recover from scope disconnect
			StartScopeReconnectThread();
		}
		else if ((m_MicroControllerState != eConnected)
			&& (m_MicroControllerState != eReflashing))
		{
			// Reconnect to micro controller if not already attempting
			m_SystemStatus.m_ControllerStatusString = "Connecting...";
			StartMicroControllerReconnectThread();
		}
#else
		m_SystemStatus.m_ControllerVersion = "Simulated";
		m_SystemStatus.m_ControllerStatus = eOK;
#endif
		
		CompStatus systemStatus = m_SystemStatus.GetOverallSystemStatus();
		if (systemStatus != prevSystemStatus)
		{
			LOG4CXX_INFO(logger, "System Status change from " << SystemStatus::GetCompStatusString(prevSystemStatus) 
				<< " to " << SystemStatus::GetCompStatusString(systemStatus));

			prevSystemStatus = systemStatus;

			theApp.GetMainDlg()->RedrawDisplay();
		}


		if (m_pSystemManagerDlg)
		{
			m_pSystemManagerDlg->PostMessage(UWM_UPDATE_SYSTEM_STATUS_MSG, (WPARAM)&m_SystemStatus);

			if (bResetUSBDevices)
			{
				m_pSystemManagerDlg->PostMessage(UWM__MSG_RESET_USB_CHIPS);
				return 0;
			}
		}
	}

	LOG4CXX_INFO(logger, "Monitor thread is exiting");

	return 0;
}

void CSystemManager::SetMicroControllerReflashing(bool val) 
{ 
	if (val)
	{
		m_MicroControllerState = eReflashing;
		StopCapture();
	}
	else
	{
		m_MicroControllerState = eReflashComplete;
	}
}

void CSystemManager::StartMicroControllerReconnectThread()
{
	DWORD ret = WaitForSingleObject(m_hMicroControllerReconnectThread, 0);
	if ((ret == WAIT_OBJECT_0) || (m_hMicroControllerReconnectThread == NULL))
	{
		DWORD dwThreadID;
		m_hMicroControllerReconnectThread = CreateThread(
			NULL,         // default security attributes
			0,            // default stack size
			(LPTHREAD_START_ROUTINE)&CSystemManager::MicroControllerReconnectThreadProc,
			(LPVOID)this,         // no thread function arguments
			0,            // default creation flags
			&dwThreadID); // receive thread identifier
	}
}

UINT CSystemManager::MicroControllerReconnectThread()
{
	CSplashThread *pSplash = theApp.GetSplashThread();
	pSplash->SetText("Verify Processor Box Power is ON");
	pSplash->ShowSplash();

	m_pMicroController->Connect();
	if (m_pMicroController->GetConnectedComPort() > 0)
	{
		pSplash->HideSplash();

		StopMonitor();

		if (m_MicroControllerState == eNoPower)
		{
			// Give time for the USB chips to come up after power up
			pSplash->SetText("Processor Box is Powering Up");
			pSplash->ShowSplash();
			Sleep(5000);  // Give time for the USB chips to come up after power up
		}

		m_MicroControllerState = eConnected;

		Start(false);
	}

	return 0;
}

void CSystemManager::StartScopeReconnectThread()
{
	LOG4CXX_INFO(logger, "StartScopeReconnectThread waiting for reconnect thread to close");

	DWORD ret = WaitForSingleObject(m_hScopeReconnectThread, 0);

	LOG4CXX_INFO(logger, "StartScopeReconnectThread starting new reconnect thread");

	if ((ret == WAIT_OBJECT_0) || (m_hScopeReconnectThread == NULL))
	{
		DWORD dwThreadID;
		m_hScopeReconnectThread = CreateThread(
			NULL,         // default security attributes
			0,            // default stack size
			(LPTHREAD_START_ROUTINE)&CSystemManager::ScopeReconnectThreadProc,
			(LPVOID)this,         // no thread function arguments
			0,            // default creation flags
			&dwThreadID); // receive thread identifier
	}
}

UINT CSystemManager::ScopeReconnectThread()
{
	CSplashThread *pSplash = theApp.GetSplashThread();
	pSplash->SetText("Verify Scope is Connected");
	pSplash->ShowSplash();

	if (m_pScopeStatus->IsScopeConnected())
	{
		pSplash->HideSplash();

		StopMonitor();

		m_MicroControllerState = eConnected;

		Start(false);
	}

	return 0;
}

CScopeSettings* CSystemManager::ReadScopeSettings()
{
	CScopeSettings *pSettings = NULL;
	std::string jsonSettings = m_pMicroController->ReadEEProm(Scope);

	pSettings = new CScopeSettings(jsonSettings);

	return pSettings;
}

int CSystemManager::WriteScopeSettings(CScopeSettings *pSettings)
{
	int retVal = -1;
	if (pSettings)
	{
		std::string currentJson = pSettings->GetEEPromString();
		retVal = m_pMicroController->WriteEEProm(Scope, currentJson);
	}

	return retVal;
}

CProcessorBoxSettings* CSystemManager::ReadProcessorBoxSettings()
{
	CProcessorBoxSettings *pSettings = NULL;
	std::string jsonSettings = m_pMicroController->ReadEEProm(ProcessorBox);

	pSettings = new CProcessorBoxSettings(jsonSettings);

	return pSettings;
}

int CSystemManager::WriteProcessorBoxSettings(CProcessorBoxSettings *pSettings)
{
	int retVal = -1;
	if (pSettings)
	{
		std::string currentJson = pSettings->GetEEPromString();
		retVal = m_pMicroController->WriteEEProm(Scope, currentJson);
	}

	return retVal;
}

bool CSystemManager::IsScopeEEPromCurrent()
{
	bool bSame = false;
	if (m_pScopeEEPromSettings && m_pScopeCurrentSettings)
	{
		std::string eepromJson = m_pScopeEEPromSettings->GetEEPromString();
		std::string currentJson = m_pScopeCurrentSettings->GetEEPromString();
		bSame = (eepromJson == currentJson);
	}

	return bSame;
}

void CSystemManager::ReadEEPromSettings()
{
	if (m_pScopeEEPromSettings)
		delete m_pScopeEEPromSettings;

	m_pScopeEEPromSettings = ReadScopeSettings();

	LOG4CXX_INFO(logger, "Read scope config from EEPROM with " << m_pScopeEEPromSettings->GetNumCameras() << " cameras");

	if (m_pProcessorEEPromSettings)
		delete m_pProcessorEEPromSettings;

	m_pProcessorEEPromSettings = ReadProcessorBoxSettings();
	m_USBReader.EnableFrontSplitImageDetection(m_pProcessorEEPromSettings->IsFrontSplitImageDetectionEnabled());
}

int CSystemManager::WriteCurrentToEEProm()
{
	int retVal = WriteScopeSettings(m_pScopeCurrentSettings);
	
	ReadEEPromSettings();

	return retVal;
}

void CSystemManager::UpdateScopeStatus(BYTE status)
{ 
	
}

std::vector<double> CSystemManager::GetMeasurements(Measurement type)
{
	vector<double> measurments;

	for (int i = 0; i < 5; i++)
	{
		m_vCameras[i]->SetMeasurementType(type);
	}

	// Wait for measurements
	Sleep(500);

	for (int i = 0; i < 5; i++)
	{
		measurments.push_back(m_vCameras[i]->GetMeasurement());
		m_vCameras[i]->SetMeasurementType(eNoMeasurement);
	}

	return measurments;
}

