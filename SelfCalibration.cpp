#include "stdafx.h"
#include "SystemManager.h"
#include "SelfCalibration.h"
#include "NGEuserDlg.h"
#include "NGEuserApp.h"
#include <fstream>

#define CAL_FOLDER "C:\\NGE\\Calibration"

using namespace std;

std::istream& ignoreline(std::ifstream& in, std::ifstream::pos_type& pos)
{
	pos = in.tellg();
	return in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string getLastLine(std::ifstream& in)
{
	std::ifstream::pos_type pos = in.tellg();

	std::ifstream::pos_type lastPos;
	while (in >> std::ws && ignoreline(in, lastPos))
		pos = lastPos;

	in.clear();
	in.seekg(pos);

	std::string line;
	std::getline(in, line);
	return line;
}


CSelfCalibration::CSelfCalibration(CSystemManager *pSystemManager)
{
	m_pSystemManager = pSystemManager;
	m_pSelfCalibrationDlg = NULL;
	m_hThread = NULL;
	m_bIsCalibrating = false;

	// Create the folder if it doesn't exist
	CreateDirectory(_T(CAL_FOLDER), NULL);

#if 0
	// Parse calibration file for last results
	string fileName = CAL_FOLDER;
	fileName += "\\CalibrationHistory.csv";
	ifstream file(fileName);

	if (file.is_open())
	{
		string result = getLastLine(file);
		if (result != SelfCalibrationStatus::GetCSVHeader())
		{
			m_LastSelfCalibration.FromCSVLine(result);
		}
		file.close();
	}
	else
	{
		// Create a new file
		ofstream fileOut(fileName);
		fileOut << SelfCalibrationStatus::GetCSVHeader() << endl;
		fileOut.close();
	}
#endif
}


CSelfCalibration::~CSelfCalibration()
{
}

void CSelfCalibration::SetSelfCalibrationDlg(CSelfCalibrationDlg *pDlg)
{ 
	m_pSelfCalibrationDlg = pDlg; 

	if (m_pSelfCalibrationDlg)
	{
		m_pSelfCalibrationDlg->SendMessage(UWM_LAST_SELF_CALIBRATION_MSG, (WPARAM)&m_LastSelfCalibration);
	}
}

void CSelfCalibration::ResetCalibration()
{
	m_LastSelfCalibration = SelfCalibrationStatus();
	UpdateDialog(&m_LastSelfCalibration);
}

bool CSelfCalibration::StartCalibration()
{
	int retVal = theApp.ShowMessageBox(_T("Self Calibration will modify settings.\nContinue with 360 System Calibration?"), _T("Warning"), MB_OKCANCEL | MB_TOPMOST);
	if (retVal == IDCANCEL)
		return false;

	DWORD dwThreadID;
	m_hThread = CreateThread(
		NULL,         // default security attributes
		0,            // default stack size
		(LPTHREAD_START_ROUTINE)&CSelfCalibration::ThreadProc,
		(LPVOID)this,         // no thread function arguments
		0,            // default creation flags
		&dwThreadID); // receive thread identifier

	return true;
}

UINT CSelfCalibration::SelfCalibration()
{
	m_bIsCalibrating = true;

	m_CurrentSelfCalibration = SelfCalibrationStatus();
	m_CurrentSelfCalibration.m_Settings = m_SelfCalibrationSettings;

	UpdateDialog(&m_CurrentSelfCalibration);

	try
	{	
		// Store previous light levels
		CLightControl *pLightControl = m_pSystemManager->GetLightControl();
		bool prevFrontOn = pLightControl->GetFrontLightsOn();
		int prevFrontColor = pLightControl->GetCurrentFrontColor();
		float prevFrontLevel = pLightControl->GetFrontLightsLevel();
		bool prevFrontAuto = pLightControl->GetFrontLightsAuto();

		bool prevSideOn = pLightControl->GetSideLightsOn();
		int prevSideColor = pLightControl->GetCurrentSideColor();
		float prevSideLevel = pLightControl->GetSideLightsLevel();
		bool prevSideAuto = pLightControl->GetSideLightsAuto();

		int prevShadowCastDelay = pLightControl->GetShadowCastDelayMs();

		// Turn off the auto lighting
		pLightControl->SetFrontLightsAuto(false, true);
		pLightControl->SetSideLightsAuto(false, true);

		// Store the previous camera settings
		std::vector<vector<cv::Mat>> previousFPNCal;
		std::vector<CCameraSettings*> previousCameraSettings;
		for (int i = 0; i < 5; i++)
		{
			CameraType cameraType = ((CameraType)i);
			CameraData *pCamera = m_pSystemManager->GetCameraData(cameraType);

			CCameraSettings *pPrevSettings = new CCameraSettings();
			pPrevSettings->Copy(pCamera->GetCameraSettings());
			previousCameraSettings.push_back(pPrevSettings);

			previousFPNCal.push_back(pCamera->GetFPNCalData());

			// Set the camera gain and exposure
			pCamera->EnableAutoExposure(false);
			pCamera->EnableAutoGain(false);
			pCamera->SetExposure(m_SelfCalibrationSettings.m_nCameraExposure);
			pCamera->SetGain(m_SelfCalibrationSettings.m_nCameraGain);
		}


		

		// Run the tests
		VerifyCameras();

		VerifyCalCup();
		
		VerifyLights();
		
		FixedPatternNoiseCal();
		
		WhiteBalanceCal();
		
		// Restore previous light levels
		pLightControl->SetShadowCastDelayMs(prevShadowCastDelay);
		pLightControl->SetShadowCastOn(false);

		pLightControl->SetCurrentFrontColor(prevFrontColor);
		pLightControl->SetFrontLightsLevel(prevFrontLevel, true);
		pLightControl->SetFrontLightsOn(prevFrontOn, true);
		pLightControl->SetFrontLightsAuto(prevFrontAuto, true);


		pLightControl->SetCurrentSideColor(prevSideColor);
		pLightControl->SetSideLightsLevel(prevSideLevel, true);
		pLightControl->SetSideLightsOn(prevSideOn, true);
		pLightControl->SetSideLightsAuto(prevSideAuto, true);

		// Write results
		string outputFolder = CAL_FOLDER;
		outputFolder += "\\" + SelfCalibrationStatus::GetTimestamp(m_CurrentSelfCalibration.m_Time);
		CreateDirectoryA(outputFolder.c_str(), NULL);
		string outputFile = outputFolder + "\\CalibrationResults.json";
		m_pSystemManager->SaveConfig(outputFile.c_str());
		for (int i = 0; i < 5; i++)
		{
			string filename = outputFolder + "\\CalCup_" + CameraNames[i] + ".bmp";
			cv::imwrite(filename, m_CurrentSelfCalibration.m_ImagesCalCup[i]);
		}

		// Determine overall results
		if ((m_CurrentSelfCalibration.m_CamerasCalStatus == eOK)
			&& (m_CurrentSelfCalibration.m_CalCupCalStatus == eOK)
			&& (m_CurrentSelfCalibration.m_LightsCalStatus == eOK)
			&& (m_CurrentSelfCalibration.m_FPNCalStatus == eOK)
			&& (m_CurrentSelfCalibration.m_WBCalStatus == eOK))
		{
			m_CurrentSelfCalibration.m_OverallCalStatus = eOK;

			// If successful, reload camera gain and exposure settings
			for (int i = 0; i < 5; i++)
			{
				CameraType cameraType = ((CameraType)i);
				CameraData *pCamera = m_pSystemManager->GetCameraData(cameraType);
				pCamera->SetExposure(previousCameraSettings[i]->GetExposure());
				pCamera->SetGain(previousCameraSettings[i]->GetGain());
				pCamera->EnableAutoExposure(previousCameraSettings[i]->IsAutoExposureEnabled());
				pCamera->EnableAutoGain(previousCameraSettings[i]->IsAutoGainEnabled());
			}
		}
		else
		{
			m_CurrentSelfCalibration.m_OverallCalStatus = eError;

			// If not successful, reload camera settings
			for (int i = 0; i < 5; i++)
			{
				CameraType cameraType = ((CameraType)i);
				CameraData *pCamera = m_pSystemManager->GetCameraData(cameraType);
				pCamera->GetCameraSettings()->Copy(previousCameraSettings[i]);
				pCamera->SetFPNCalData(previousFPNCal[i]);
				pCamera->RefreshSensorSettings();
			}
		}

		// Write results
		m_CurrentSelfCalibration.SaveJson(outputFile);

		// Send status to CEF
		theApp.SendCalibrationStatus({
			m_CurrentSelfCalibration.m_CamerasCalStatus,
			m_CurrentSelfCalibration.m_LightsCalStatus,
			m_CurrentSelfCalibration.m_CalCupCalStatus,
			m_CurrentSelfCalibration.m_FPNCalStatus,
			m_CurrentSelfCalibration.m_WBCalStatus,
			m_CurrentSelfCalibration.m_OverallCalStatus 
		});

	}
	catch (...)
	{
		theApp.ShowMessageBox(_T("Error occurred during 360 System Calibration. Please retry."), _T("Error"), MB_OK | MB_TOPMOST);
	}

	// Update history
	string fileName = CAL_FOLDER;
	fileName += "\\CalibrationHistory.csv";
	ofstream fileOut(fileName, ios_base::app);
	if (fileOut.is_open())
	{
		fileOut << m_CurrentSelfCalibration.GetCSVLine() << endl;
		fileOut.close();
	}
	else
	{
		string msg = "Unable to update" + fileName + "!";
		theApp.ShowMessageBox(msg.c_str(), "Error", MB_OK | MB_TOPMOST);
	}

	UpdateDialog(&m_CurrentSelfCalibration);

	m_LastSelfCalibration = m_CurrentSelfCalibration;
	if (m_pSelfCalibrationDlg)
	{
		m_pSelfCalibrationDlg->SendMessage(UWM_LAST_SELF_CALIBRATION_MSG, (WPARAM)&m_LastSelfCalibration);
	}	

	m_bIsCalibrating = false;

	return 0;
}

void CSelfCalibration::UpdateDialog(SelfCalibrationStatus *pStatus)
{
	if (m_pSelfCalibrationDlg)
	{
		m_pSelfCalibrationDlg->SendMessage(UWM_UPDATE_SELF_CALIBRATION_MSG, (WPARAM)pStatus);
	}
}

void CSelfCalibration::VerifyCameras()
{
	// Verify all cameras are operational
	m_CurrentSelfCalibration.m_SystemStatus = m_pSystemManager->GetSystemStatus();

#ifndef IGNORE_MICROCONTROLLER 
	if (m_CurrentSelfCalibration.m_SystemStatus.GetOverallSystemStatus() != eOK)
	{
		m_CurrentSelfCalibration.m_CamerasCalStatus = eError;
	}
	else
	{
		m_CurrentSelfCalibration.m_CamerasCalStatus = eOK;
	}
#else
	m_CurrentSelfCalibration.m_CamerasCalStatus = eOK;
#endif

	UpdateDialog(&m_CurrentSelfCalibration);
}

void CSelfCalibration::VerifyCalCup()
{
	CLightControl *pLightControl = m_pSystemManager->GetLightControl();

	// Turn lights on at level and verify cup
	pLightControl->SetFrontLightsOn(true, true);
	pLightControl->SetSideLightsOn(true, true);
	pLightControl->SetCustomColor(RGB(255, 255, 255));
	pLightControl->SetFrontLightsLevel(m_SelfCalibrationSettings.m_fCalCupFrontLightLevel, true);
	pLightControl->SetSideLightsLevel(m_SelfCalibrationSettings.m_fCalCupSideLightLevel, true);

	m_CurrentSelfCalibration.m_CalCupDeviations = m_pSystemManager->GetMeasurements(eDeviation);

#ifndef IGNORE_MICROCONTROLLER 
	bool bLimitExceeded = false;
	for (int i = 0; i < 5; i++)
	{
		if (m_CurrentSelfCalibration.m_CalCupDeviations[i] > m_SelfCalibrationSettings.m_dCalCupMaxDeviation)
		{
			bLimitExceeded = true;
			break;
		}
	}
	m_CurrentSelfCalibration.m_CalCupCalStatus = bLimitExceeded ? eError : eOK;
#else
	m_CurrentSelfCalibration.m_CalCupCalStatus = eOK;
#endif

	for (int i = 0; i < 5; i++)
	{
		CameraType cameraType = ((CameraType)i);
		CameraData *pCamera = m_pSystemManager->GetCameraData(cameraType);
		m_CurrentSelfCalibration.m_ImagesCalCup.push_back(pCamera->GetCameraImage());
	}

	UpdateDialog(&m_CurrentSelfCalibration);
}

void CSelfCalibration::VerifyLights()
{
	CLightControl *pLightControl = m_pSystemManager->GetLightControl();

	pLightControl->SetFrontLightsLevel(1.0f, true);
	pLightControl->SetSideLightsLevel(1.0f, true);

	// Setup the pure the color values
	vector<COLORREF> colorValues = vector<COLORREF>(3);
	colorValues[RED] = RGB(255, 0, 0);
	colorValues[GREEN] = RGB(0, 255, 0);
	colorValues[BLUE] = RGB(0, 0, 255);

	// Enable slow shadow casting and verify each pair of lights
	pLightControl->SetShadowCastOn(true);
	pLightControl->SetShadowCastDelayMs(m_SelfCalibrationSettings.m_nLightCheckShadowCastDelay);
	for (int i = 0; i < 5; i++)
	{
		CameraType cameraType = ((CameraType)i);
		CameraData *pCamera = m_pSystemManager->GetCameraData(cameraType);

		std::set<CameraType> group1, group2;
		group1.insert(cameraType);
		group2.insert(cameraType);
		pLightControl->SetShadowCastGroups(group1, group2);

		for (int j = 0; j < 3; j++)
		{
			pLightControl->SetCustomColor(colorValues[j]);

			pCamera->SetMeasurementType(eLightResponse);
			Sleep(m_SelfCalibrationSettings.m_nLightCheckTime);
			m_CurrentSelfCalibration.m_LightResponses[i][j] = pCamera->GetMeasurement();
			pCamera->SetMeasurementType(eNoMeasurement);
		}
	}
	pLightControl->SetCustomColor(RGB(0,0,0));
	pLightControl->SetShadowCastOn(false);

#if 1//ndef IGNORE_MICROCONTROLLER 
	bool bLimitExceeded = false;
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (m_CurrentSelfCalibration.m_LightResponses[i][j] < m_SelfCalibrationSettings.m_fLightCheckMinValue)
			{
				bLimitExceeded = true;
				break;
			}
		}
	}
	m_CurrentSelfCalibration.m_LightsCalStatus = bLimitExceeded ? eError : eOK;
#else
	m_CurrentSelfCalibration.m_LightsCalStatus = eOK;
#endif

	UpdateDialog(&m_CurrentSelfCalibration);
}

void CSelfCalibration::FixedPatternNoiseCal()
{
	CLightControl *pLightControl = m_pSystemManager->GetLightControl();

	// Do FPN for each side camera
	pLightControl->SetFrontLightsOn(false, true);
	pLightControl->SetSideLightsOn(false, true);

#ifndef IGNORE_MICROCONTROLLER 
	// Start the calibration on all cameras
	for (int i = 0; i < 5; i++)
	{
		CameraType cameraType = ((CameraType)i);
		if (cameraType != eFront)
		{
			m_pSystemManager->GetCameraData(cameraType)->CalibrateFPN();
		}
	}

	// Wait for completion
	time_t start, now;
	time(&start);
	for (int i = 0; i < 5; i++)
	{
		CameraType cameraType = ((CameraType)i);
		if (cameraType != eFront)
		{
			bool timeout = false;
			while (m_pSystemManager->GetCameraData(cameraType)->IsFPNCalActive())
			{
				Sleep(100);
				time(&now);
				double seconds = difftime(now, start);
				if (seconds > m_SelfCalibrationSettings.m_nFPNTimeoutSecs)
				{
					timeout = true;
					break;
				}
			}

			if (timeout)
			{
				// If there was a timeout, abort FPN calibration
				m_pSystemManager->GetCameraData(cameraType)->AbortFPNCal();
			}
			else
			{
				m_CurrentSelfCalibration.m_FPNMaxValues[i] = m_pSystemManager->GetCameraData(cameraType)->GetMaxFPNValue();
			}
		}
		else
		{
			m_CurrentSelfCalibration.m_FPNMaxValues[i] = 0;
		}
	}

	bool bLimitExceeded = false;
	for (int i = 0; i < 5; i++)
	{
		if (m_CurrentSelfCalibration.m_FPNMaxValues[i] > m_SelfCalibrationSettings.m_nFPNMaxValueLimit)
		{
			bLimitExceeded = true;
			break;
		}
	}
	m_CurrentSelfCalibration.m_FPNCalStatus = bLimitExceeded ? eError : eOK;
#else
	m_CurrentSelfCalibration.m_FPNCalStatus = eOK;
#endif

	UpdateDialog(&m_CurrentSelfCalibration);
}

void CSelfCalibration::WhiteBalanceCal()
{
	CLightControl *pLightControl = m_pSystemManager->GetLightControl();

	// Do White Balance
	pLightControl->SetShadowCastOn(false);
	pLightControl->SetCustomColor(RGB(255, 255, 255));
	pLightControl->SetFrontLightsOn(true, true);
	pLightControl->SetSideLightsOn(true, true);
	pLightControl->SetFrontLightsLevel(m_SelfCalibrationSettings.m_fCalCupFrontLightLevel, true);
	pLightControl->SetSideLightsLevel(m_SelfCalibrationSettings.m_fCalCupSideLightLevel, true);

	// Wait for lights to stabilize
	Sleep(m_SelfCalibrationSettings.m_nWBSettleDelay);

	// Store the camera RGB values
	for (int i = 0; i < 5; i++)
	{
		CameraType cameraType = (CameraType)i;
		CameraData *pCamera = m_pSystemManager->GetCameraData(cameraType);

		double red, green, blue;
		pCamera->GetLastChannelValues(red, green, blue);
		if (green != 0)
		{
			m_CurrentSelfCalibration.m_WhiteBalanceGains[i][RED] = red / green;
			m_CurrentSelfCalibration.m_WhiteBalanceGains[i][GREEN] = green / green;
			m_CurrentSelfCalibration.m_WhiteBalanceGains[i][BLUE] = blue / green;
		}
		else
		{
			m_CurrentSelfCalibration.m_WhiteBalanceGains[i][RED] = FLT_MAX;
			m_CurrentSelfCalibration.m_WhiteBalanceGains[i][GREEN] = FLT_MAX;
			m_CurrentSelfCalibration.m_WhiteBalanceGains[i][BLUE] = FLT_MAX;
		}
	}

#ifndef IGNORE_MICROCONTROLLER 
	bool bLimitExceeded = false;

	if (IsOutsideLimits(m_CurrentSelfCalibration.m_WhiteBalanceGains[0][RED], m_SelfCalibrationSettings.m_fWBFrontCompRed, m_SelfCalibrationSettings.m_fWBTolerance)
		|| IsOutsideLimits(m_CurrentSelfCalibration.m_WhiteBalanceGains[0][GREEN], m_SelfCalibrationSettings.m_fWBFrontCompGreen, m_SelfCalibrationSettings.m_fWBTolerance)
		|| IsOutsideLimits(m_CurrentSelfCalibration.m_WhiteBalanceGains[0][BLUE], m_SelfCalibrationSettings.m_fWBFrontCompBlue, m_SelfCalibrationSettings.m_fWBTolerance))
	{
		bLimitExceeded = true;
	}

	for (int i = 1; i < 5; i++)
	{
		if (IsOutsideLimits(m_CurrentSelfCalibration.m_WhiteBalanceGains[i][RED], m_SelfCalibrationSettings.m_fWBSideCompRed, m_SelfCalibrationSettings.m_fWBTolerance)
			|| IsOutsideLimits(m_CurrentSelfCalibration.m_WhiteBalanceGains[i][GREEN], m_SelfCalibrationSettings.m_fWBSideCompGreen, m_SelfCalibrationSettings.m_fWBTolerance)
			|| IsOutsideLimits(m_CurrentSelfCalibration.m_WhiteBalanceGains[i][BLUE], m_SelfCalibrationSettings.m_fWBSideCompBlue, m_SelfCalibrationSettings.m_fWBTolerance))
		{
			bLimitExceeded = true;
		}
	}
	m_CurrentSelfCalibration.m_WBCalStatus = bLimitExceeded ? eError : eOK;
#else
	m_CurrentSelfCalibration.m_WBCalStatus = eOK;
#endif

	UpdateDialog(&m_CurrentSelfCalibration);
}

bool CSelfCalibration::IsOutsideLimits(float value, float limit, float tolerance)
{
	return ((value < (limit - tolerance)) || (value > (limit + tolerance)));
}
