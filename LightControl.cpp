#include "stdafx.h"
#include "LightControl.h"
#include "CameraData.h"
#include "MicroController.h"
#include "NGEuserApp.h"
#include "LightingDlg.h"
#include <fstream>
#include <sstream>
#include "json\json.h"

#define LIGHT_CONFIG "LightConfig.txt"
#define APP_LIGHTS "Lights"

#define DEMO_CYCLE_MS 1000
#define NUM_LIGHT_CHANNELS 30
#define MIN_LIGHT_GAIN_CHANGE 0.2
#define MAX_LIGHT_GAIN_CHANGE 0.8

log4cxx::LoggerPtr  CLightControl::logger(log4cxx::Logger::getLogger("LightControl"));

CLightControl::CLightControl(CMicroController *pMicroController)
{
	m_pMicroController = pMicroController;

	m_pUpdateCallbackObject = NULL;
	m_UpdateLightCallback = NULL;
	m_hAutoFrontThread = NULL;
	m_hAutoSideThread = NULL;
	m_hDemoThread = NULL;
	m_bExitAutoFrontThread = false;
	m_bExitAutoSideThread = false;
	m_bExitDemoThread = false;

	m_fControlGamma = 0.5;

	m_bAutoSatOrAvg = true;
	m_AutoMode = LIGHTS;
	m_fAutoSatSetpoint = 0.02;
	m_fAutoAvgSetpoint = 150;
	m_fAutoSatGainP = 5;
	m_fAutoAvgGainP = 0.005;
	m_fAutoSatGainI = 0;
	m_fAutoAvgGainI = 0;
	m_fAutoSatGainD = 0;
	m_fAutoAvgGainD = 0;
	m_fAutoPeriodMs = 500;

	_autoLightData.resize(5);
	_autoStartTimes.resize(5);
}

CLightControl::~CLightControl()
{
	StopAutoFrontThread();
	StopAutoSideThread();
	StopDemoThread();
	SetFrontLightsOn(false, true);
	SetSideLightsOn(false, true);
}

void CLightControl::ReloadSettings(string folder)
{
	char value[_MAX_PATH];
	string fname = folder + "\\" + "LightConfig.json";
	
	StopAutoFrontThread();
	StopAutoSideThread();
	StopDemoThread();
	
	m_ColorValues.clear();
	m_ColorValues[0] = RGB(255, 255, 255);
	/*m_ColorValues[1] = RGB(255, 0, 0);
	m_ColorValues[2] = RGB(255, 0, 255);
	m_ColorValues[3] = RGB(0, 0, 255);
	m_ColorValues[4] = RGB(0, 255, 255);
	m_ColorValues[5] = RGB(0, 255, 0);
	m_ColorValues[6] = RGB(255, 255, 0);
	m_ColorValues[7] = RGB(255, 255, 0);*/

	m_bFrontLightsOn = true;
	m_bSideLightsOn = true;
	m_bFrontLightsAuto = false;
	m_bSideLightsAuto = false;

	m_ColorSelectBandFront = RGB(0, 255, 0);
	m_ColorSelectBandSide = RGB(0, 255, 0);
	m_bSelectBandImaging = false;

	bool m_bCustomColorEnabled = false;

	m_CameraPrevSat.resize(5);
	m_CameraPrevAvg.resize(5);
	m_CameraIntegralSat.resize(5);
	m_CameraIntegralAvg.resize(5);
	m_CameraLightValues.resize(5);
	for (int camera = 0; camera < 5; camera++)
	{
		m_CameraLightValues[camera].resize(2);
		for (int side = 0; side < 2; side++)
		{
			if ((CameraType)camera == eFront)
			{
				m_FrontChannels.push_back(camera);
			}
			else
			{
				m_SideChannels.push_back(camera);
			}
		}
	}


	std::ifstream file(fname);

	Json::Value root;
	Json::Reader reader;

	bool parsingSuccessful = reader.parse(file, root);
	if (parsingSuccessful)
	{
		m_nCurrentFrontColor = root.get("FrontColor", 0).asInt();
		m_nCurrentSideColor = root.get("SideColor", 0).asInt();

		m_fFrontLightsLevel = root.get("FrontLightsLevel", 1).asFloat();
		m_fSideLightsLevel = root.get("SideLightsLevel", 1).asFloat();

		m_bFrontLightsAuto = (root.get("FrontLightsAuto", 1).asInt() == 1);
		m_bSideLightsAuto = (root.get("SideLightsAuto", 1).asInt() == 1);

		m_fControlGamma = root.get("ControlGamma", 1).asFloat();

		m_bAutoSatOrAvg = (root.get("AutoSatOrAvg", 1).asInt() == 1);
		m_AutoMode = (AutoMode)(root.get("AutoMode", 2).asInt());
		m_fAutoSatSetpoint = root.get("AutoSatSetpoint", 0.05).asFloat();
		m_fAutoAvgSetpoint = root.get("AutoAvgSetpoint", 150).asFloat();
		m_fAutoSatGainP = root.get("AutoSatGainP", 1.5).asFloat();
		m_fAutoAvgGainP = root.get("AutoAvgGainP", 0.004).asFloat();
		m_fAutoSatGainI = root.get("AutoSatGainI", 0).asFloat();
		m_fAutoAvgGainI = root.get("AutoAvgGainI", 0).asFloat();
		m_fAutoSatGainD = root.get("AutoSatGainD", 0).asFloat();
		m_fAutoAvgGainD = root.get("AutoAvgGainD", 0).asFloat();
		m_fAutoPeriodMs = root.get("AutoPeriodMs", 100).asFloat();
		
		const Json::Value& arrayColorPresets = root["Color Presets"];
		int index = 0;
		for (Json::ValueConstIterator itr = arrayColorPresets.begin(); itr != arrayColorPresets.end(); itr++, index++)
		{
			m_ColorValues[index] = RGB(itr->get("R", 0).asInt(),
									itr->get("G", 0).asInt(),
									itr->get("B", 0).asInt());
		}

		Json::Value narrowBand = root["Select Band Front"];
		if (!narrowBand.empty())
		{
			m_ColorSelectBandFront = RGB(narrowBand.get("R", 0).asInt(),
				narrowBand.get("G", 0).asInt(),
				narrowBand.get("B", 0).asInt());
		}

		narrowBand = root["Select Band Side"];
		if (!narrowBand.empty())
		{
			m_ColorSelectBandSide = RGB(narrowBand.get("R", 0).asInt(),
				narrowBand.get("G", 0).asInt(),
				narrowBand.get("B", 0).asInt());
		}

		m_LightCalSettings.Load(root);

		for (int camera = 0; camera < 5; camera++)
		{
			for (int side = 0; side < 2; side++)
			{
				std::stringstream lightName;
				lightName << "LightValue_" << CameraNames[camera] << (side ? "_L" : "_R");
				float value = root.get(lightName.str(), 1).asFloat();
				SetCameraValue((CameraType)camera, (LightSide)side, value, false);
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(logger, "Unable to parse " << fname);
	}

	file.close();

	// Read gain lookup table
	_gainLookup.clear();
	fname = folder + "\\" + "SideGainLookup.txt";
	std::ifstream fileLookup(fname);
	if (fileLookup.is_open())
	{
		while (!fileLookup.eof())
		{
			float val;
			fileLookup >> val;
			_gainLookup.push_back(val);
		}
		fileLookup.close();
	}
		
	m_bShadowCastOn = false;
	m_nShadowCastDelayMs = 100;
	for (int i = 0; i < 5; i++)
	{
		m_ShadowCastGroup1.insert((CameraType)i);
		m_ShadowCastGroup2.insert((CameraType)i);
	}
}

void CLightControl::SaveSettings(string folder)
{
	string fname = folder + "\\" + "LightConfig.json";

	Json::Value root;
	Json::Value arrayInputPoints;

	root["FrontColor"] = m_nCurrentFrontColor;
	root["SideColor"] = m_nCurrentSideColor;

	root["FrontLightsLevel"] = m_fFrontLightsLevel;
	root["SideLightsLevel"] = m_fSideLightsLevel;

	root["FrontLightsAuto"] = (m_bFrontLightsAuto ? 1 : 0);
	root["SideLightsAuto"] = (m_bSideLightsAuto ? 1 : 0);

	root["ControlGamma"] = m_fControlGamma;

	root["AutoSatOrAvg"] = (m_bAutoSatOrAvg ? 1 : 0);
	root["AutoMode"] = (int)m_AutoMode;
	root["AutoSatSetpoint"] = m_fAutoSatSetpoint;
	root["AutoAvgSetpoint"] = m_fAutoAvgSetpoint;
	root["AutoSatGainP"] = m_fAutoSatGainP;
	root["AutoAvgGainP"] = m_fAutoAvgGainP;
	root["AutoSatGainI"] = m_fAutoSatGainI;
	root["AutoAvgGainI"] = m_fAutoAvgGainI;
	root["AutoSatGainD"] = m_fAutoSatGainD;
	root["AutoAvgGainD"] = m_fAutoAvgGainD;
	root["AutoPeriodMs"] = m_fAutoPeriodMs;

	Json::Value arrayColorPresets;
	for (int index = 0; index < m_ColorValues.size(); index++)
	{
		Json::Value colorValue;
		colorValue["R"] = GetRValue(m_ColorValues[index]);
		colorValue["G"] = GetGValue(m_ColorValues[index]);
		colorValue["B"] = GetBValue(m_ColorValues[index]);
		arrayColorPresets.append(colorValue);
	}
	root["Color Presets"] = arrayColorPresets;

	Json::Value narrowBandFront;
	narrowBandFront["R"] = GetRValue(m_ColorSelectBandFront);
	narrowBandFront["G"] = GetGValue(m_ColorSelectBandFront);
	narrowBandFront["B"] = GetBValue(m_ColorSelectBandFront);
	root["Select Band Front"] = narrowBandFront;

	Json::Value narrowBandSide;
	narrowBandSide["R"] = GetRValue(m_ColorSelectBandSide);
	narrowBandSide["G"] = GetGValue(m_ColorSelectBandSide);
	narrowBandSide["B"] = GetBValue(m_ColorSelectBandSide);
	root["Select Band Side"] = narrowBandSide;

	for (int camera = 0; camera < 5; camera++)
	{
		for (int side = 0; side < 2; side++)
		{
			std::stringstream lightName;
			lightName << "LightValue_" << CameraNames[camera] << (side ? "_L" : "_R");
			double value = GetCameraValue((CameraType)camera, (LightSide)side);

			root[lightName.str()] = value;
		}
	}

	m_LightCalSettings.Save(root);

	std::ofstream file(fname);
	Json::StreamWriterBuilder styledWriterBuilder;
	styledWriterBuilder.settings_["precision"] = 5;
	file << Json::writeString(styledWriterBuilder, root);
	file.close();

	// Write gain lookup table
	if (_gainLookup.size())
	{
		fname = folder + "\\" + "SideGainLookup.txt";
		std::ofstream fileLookup(fname);
		if (fileLookup.is_open())
		{
			for (int i = 0; i < _gainLookup.size(); i++)
			{
				fileLookup << _gainLookup[i] << std::endl;
			}
			fileLookup.close();
		}
	}
}

void CLightControl::MicroControllerConnected()
{
	LOG4CXX_INFO(logger, "Redownloading settings after connection to MicroController");

	for (int camera = 0; camera < 5; camera++)
	{
		for (int side = 0; side < 2; side++)
		{
			float value = GetCameraValue((CameraType)eFront, (LightSide)side);
			SetCameraValue((CameraType)camera, (LightSide)side, value, true);
		}
	}

	// Update the color (this will update everything)
	SetCurrentFrontColor(m_nCurrentFrontColor, true);
	SetCurrentSideColor(m_nCurrentSideColor, true);

	SetFrontLightsOn(m_bFrontLightsOn, true);
	SetFrontLightsAuto(m_bFrontLightsAuto, true);
	SetSideLightsOn(m_bSideLightsOn, true);
	SetSideLightsAuto(m_bSideLightsAuto, true);
	UpdateShadowCast();
}

void CLightControl::SetUpdateLightCallBack(UpdateLightCallbackType callback, void *pObject)
{
	m_UpdateLightCallback = callback;
	m_pUpdateCallbackObject = pObject;
}

void CLightControl::AddSelectBandImagingCallBack(SelectBandImagingCallbackType callback, void *pObject)
{
	m_SelectBandImagingCallbacks[callback] = pObject;
}

COLORREF CLightControl::GetColorValue(int index)
{
	COLORREF value = RGB(255, 255, 255);
	if ((index >= 0) && index < m_ColorValues.size())
	{
		value = m_ColorValues[index];
	}

	return value;
}

void CLightControl::SetCurrentFrontColor(int index, bool force)
{
	if (!force && (m_nCurrentFrontColor == index) && !m_bCustomColorEnabled)
	{
		return;
	}

	if ((index >= 0) && index < m_ColorValues.size())
	{
		m_bCustomColorEnabled = false;
		m_nCurrentFrontColor = index;
		for (int side = 0; side < 2; side++)
		{
			// Get the current intensity
			float value = GetCameraValue((CameraType) eFront, (LightSide)side);

			// Update with the new color
			SetCameraValue((CameraType)eFront, (LightSide)side, value, true);
		}
	}
}

void CLightControl::SetCurrentSideColor(int index, bool force)
{
	if (!force && (m_nCurrentSideColor == index) && !m_bCustomColorEnabled)
	{
		return;
	}

	if ((index >= 0) && index < m_ColorValues.size())
	{
		m_bCustomColorEnabled = false;
		m_nCurrentSideColor = index;
		for (int camera = 1; camera < 5; camera++)
		{
			for (int side = 0; side < 2; side++)
			{
				// Get the current intensity
				float value = GetCameraValue((CameraType)camera, (LightSide)side);

				// Update with the new color
				SetCameraValue((CameraType)camera, (LightSide)side, value, true);
			}
		}
	}
}

int CLightControl::LightChannelLookup(CameraType camera, LightSide side, Color color)
{
	int cameraIndex = 0;

	int usbCameraBoard = theApp.GetSystemManager()->GetCameraData(camera)->GetUSBCameraNum();
	switch (usbCameraBoard)
	{
	case 1: cameraIndex = 3; break; // Side
	case 2: cameraIndex = 4; break; // Side
	case 3: cameraIndex = 2; break; // Side
	case 4: cameraIndex = 1; break; // Side
	case 5: cameraIndex = 0; break; // MIPI Camera
	}

	return ((int)cameraIndex * 6 + (int)side * 3 + (int)color);
}

float CLightControl::GetCameraValue(CameraType camera, LightSide side)
{
	return m_CameraLightValues[(int)camera][(int)side];
}

void CLightControl::SetCameraValue(CameraType camera, LightSide side, float value, bool bUpdateMC)
{
	m_CameraLightValues[(int)camera][(int)side] = value;
	
	int channelR = LightChannelLookup(camera, side, RED);
	int channelG = LightChannelLookup(camera, side, GREEN);
	int channelB = LightChannelLookup(camera, side, BLUE);

	// Deterimine the color based on the current mode
	COLORREF color;
	if (m_bCustomColorEnabled)
	{
		color = m_CustomColor;
	}
	else if (m_bSelectBandImaging)
	{
		color = (camera == eFront? m_ColorSelectBandFront: m_ColorSelectBandSide);
	}
	else if (camera == eFront)
	{
		color = m_ColorValues[m_nCurrentFrontColor];
	}
	else
	{
		color = m_ColorValues[m_nCurrentSideColor];
	}

	// Set the color gain based on the camera and color
	float gainR, gainG, gainB;
	if (IsSingleColor(color))
	{
		gainR = gainG = gainB = 1;
	}
	else if (camera == eFront)
	{
		m_LightCalSettings.GetFrontGain(gainR, gainG, gainB);
	}
	else
	{
		m_LightCalSettings.GetSideGain(gainR, gainG, gainB);
	}
	
	float compR = GetRValue(color);
	float compG = GetGValue(color);
	float compB = GetBValue(color);

	float compMax = compR;
	compMax = max(compMax, compG);
	compMax = max(compMax, compB);

	// Do nonlinear control
	value = pow(value, m_fControlGamma);
	value = max(0.005f, value); // Don't want to turn off the lights completely

#if 0
	UpdateChannelValue(channelR, value * compR * gainR / compMax, bUpdateMC);
	UpdateChannelValue(channelG, value * compG * gainG / compMax, bUpdateMC);
	UpdateChannelValue(channelB, value * compB * gainB / compMax, bUpdateMC);
#else
	std::map<int, float> values;
	values[channelR] = value * compR * gainR / compMax;
	values[channelG] = value * compG * gainG / compMax;
	values[channelB] = value * compB * gainB / compMax;
	UpdateChannelValues(values, bUpdateMC);
#endif

	if (m_UpdateLightCallback)
	{
		m_UpdateLightCallback(m_pUpdateCallbackObject, camera, side, (int)(float)(100.0*value + 0.5));
	}
}

void CLightControl::SetFrontLightsLevel(float fLevel, bool bUpdateMC)
{ 
	fLevel = min(fLevel, 1.0f);
	fLevel = max(fLevel, 0.1f);

	if (fabs(m_fFrontLightsLevel - fLevel) > 0.01)
	{
		m_fFrontLightsLevel = fLevel;
		SetCameraValue(eFront, LEFT, fLevel, bUpdateMC);
		SetCameraValue(eFront, RIGHT, fLevel, bUpdateMC);
		
		if (bUpdateMC)
		{
			m_pMicroController->SetLEDLevel(true, (int)(m_fFrontLightsLevel * 10.0 + 0.5));
		}
	}
	
}

void CLightControl::SetSideLightsLevel(float fLevel, bool bUpdateMC)
{ 
	fLevel = min(fLevel, 1.0f);
	fLevel = max(fLevel, 0.1f);
	
	if (fabs(m_fSideLightsLevel - fLevel) > 0.01)
	{
		m_fSideLightsLevel = fLevel;
		for (int i = 0; i < 2; i++)
		{
			SetCameraValue(eLeft, (LightSide)i, fLevel, bUpdateMC);
			SetCameraValue(eRight, (LightSide)i, fLevel, bUpdateMC);
			SetCameraValue(eTop, (LightSide)i, fLevel, bUpdateMC);
			SetCameraValue(eBottom, (LightSide)i, fLevel, bUpdateMC);
		}
		if (bUpdateMC)
		{
			m_pMicroController->SetLEDLevel(false, (int)(m_fSideLightsLevel * 10.0 + 0.5));
		}
	}
}

void CLightControl::SetFrontLightsOn(bool bOn, bool bUpdateMC)
{
	m_bFrontLightsOn = bOn;
	if (bUpdateMC)
	{
		m_pMicroController->SetLEDLevel(true, m_bFrontLightsOn ? (int)(m_fFrontLightsLevel * 10.0 + 0.5) : 0);
	}
}

void CLightControl::SetSideLightsOn(bool bOn, bool bUpdateMC)
{ 
	m_bSideLightsOn = bOn; 
	if (bUpdateMC)
	{
		m_pMicroController->SetLEDLevel(false, m_bSideLightsOn ? (int)(m_fSideLightsLevel * 10.0 + 0.5) : 0);
	}
}

void CLightControl::SetFrontLightsAuto(bool bAuto, bool bUpdateMC)
{
	m_bFrontLightsAuto = bAuto;

	// Stop the old thread if it exists
	StopAutoFrontThread();

	if (m_bFrontLightsAuto)
	{
		// Create the auto thread
		DWORD dwThreadID;
		m_hAutoFrontThread = CreateThread(
			NULL,         // default security attributes
			0,            // default stack size
			(LPTHREAD_START_ROUTINE)&CLightControl::AutoFrontThreadProc,
			(LPVOID)this,         // no thread function arguments
			0,            // default creation flags
			&dwThreadID); // receive thread identifier

	}
	else
	{
		// Return values to manual values
		SetFrontLightsLevel(GetFrontLightsLevel(), true);
	}
}

void CLightControl::StopAuto()
{
	LOG4CXX_INFO(logger, "StopAuto");

	StopAutoFrontThread();
	StopAutoSideThread();
}

void CLightControl::StopAutoFrontThread()
{
	if (m_hAutoFrontThread != NULL)
	{
		LOG4CXX_INFO(logger, "StopAutoFrontThread");

		m_bExitAutoFrontThread = true;
		WaitForSingleObject(m_hAutoFrontThread, INFINITE);
		m_bExitAutoFrontThread = false;
		m_hAutoFrontThread = NULL;

		LOG4CXX_INFO(logger, "StopAutoFrontThread complete");

	}
}

void CLightControl::SetSideLightsAuto(bool bAuto, bool bUpdateMC)
{ 
	m_bSideLightsAuto = bAuto; 

	// Stop the old thread if it exists
	StopAutoSideThread();

	if (m_bSideLightsAuto)
	{
		// Create the auto thread
		DWORD dwThreadID;
		m_hAutoSideThread = CreateThread(
			NULL,         // default security attributes
			0,            // default stack size
			(LPTHREAD_START_ROUTINE)&CLightControl::AutoSideThreadProc,
			(LPVOID)this,         // no thread function arguments
			0,            // default creation flags
			&dwThreadID); // receive thread identifier

	}
	else
	{
		// Return values to manual values
		SetSideLightsLevel(GetSideLightsLevel(), true);
	}
}

void CLightControl::StopAutoSideThread()
{
	if (m_hAutoSideThread != NULL)
	{
		LOG4CXX_INFO(logger, "StopAutoSideThread");

		m_bExitAutoSideThread = true;
		WaitForSingleObject(m_hAutoSideThread, INFINITE);
		m_bExitAutoSideThread = false;
		m_hAutoSideThread = NULL;

		LOG4CXX_INFO(logger, "StopAutoSideThread complete");

	}
}

void CLightControl::UpdateLEDMUXPeriod(unsigned int period, bool bUpdateMC)
{
	if (bUpdateMC)
	{
		m_pMicroController->SetLEDMUXPeriod(period);
	}
}

void CLightControl::UpdateChannelValue(int channel, float value, bool bUpdateMC)
{
	if (bUpdateMC)
	{
		m_pMicroController->SetLEDValue(channel, value);
	}
}

void CLightControl::UpdateChannelValues(std::map<int, float> values, bool bUpdateMC)
{
	if (bUpdateMC)
	{
		m_pMicroController->SetLEDValues(values);
	}
}

void CLightControl::SetShadowCastOn(bool bOn)
{
	m_bShadowCastOn = bOn;
	UpdateShadowCast();
}

bool CLightControl::GetShadowCastGroup1(CameraType camera)
{
	return (m_ShadowCastGroup1.find(camera) != m_ShadowCastGroup1.end());
}

bool CLightControl::GetShadowCastGroup2(CameraType camera)
{
	return (m_ShadowCastGroup2.find(camera) != m_ShadowCastGroup2.end());
}

void CLightControl::UpdateShadowGroup1(CameraType camera, bool bOn)
{
	if (bOn)
	{
		m_ShadowCastGroup1.insert(camera);
	}
	else
	{
		m_ShadowCastGroup1.erase(camera);
	}
	UpdateShadowCast();
}

void CLightControl::UpdateShadowGroup2(CameraType camera, bool bOn)
{
	if (bOn)
	{
		m_ShadowCastGroup2.insert(camera);
	}
	else
	{
		m_ShadowCastGroup2.erase(camera);
	}
	UpdateShadowCast();
}

void CLightControl::SetShadowCastGroups(std::set<CameraType> &group1, std::set<CameraType> &group2)
{
	m_ShadowCastGroup1 = group1;
	m_ShadowCastGroup2 = group2;
	
	if (m_bShadowCastOn)
		UpdateShadowCast();
}

void CLightControl::SetShadowCastDelayMs(int delayMs)
{
	m_nShadowCastDelayMs = delayMs;
	
	if (m_bShadowCastOn)
		UpdateShadowCast();
}

void CLightControl::UpdateShadowCast()
{
	int delayMs = m_bShadowCastOn ? m_nShadowCastDelayMs : 0;

	std::set<CameraType>::iterator it;

	// Create goup 1 bitmask
	int bitmask1 = 0;
	LightSide side = LEFT;
	for (it = m_ShadowCastGroup1.begin(); it != m_ShadowCastGroup1.end(); ++it)
	{
		bitmask1 |= 1 << LightChannelLookup(*it, side, RED);
		bitmask1 |= 1 << LightChannelLookup(*it, side, GREEN);
		bitmask1 |= 1 << LightChannelLookup(*it, side, BLUE);
	}

	// Create goup 2 bitmask
	int bitmask2 = 0;
	side = RIGHT;
	for (it = m_ShadowCastGroup2.begin(); it != m_ShadowCastGroup2.end(); ++it)
	{
		bitmask2 |= 1 << LightChannelLookup(*it, side, RED);
		bitmask2 |= 1 << LightChannelLookup(*it, side, GREEN);
		bitmask2 |= 1 << LightChannelLookup(*it, side, BLUE);
	}
	
	m_pMicroController->SetShadowCast(delayMs, bitmask1, bitmask2);
}

UINT CLightControl::AutoFrontLoop()
{
	m_CameraIntegralSat[eFront] = 0;
	m_CameraIntegralAvg[eFront] = 0;

	//Store camera gain
	CameraData *pCamera = theApp.GetSystemManager()->GetCameraData(eFront);
	int prevGain = pCamera->GetGain();

	_autoStartTimes[(int)eFront] = clock();

	while (!m_bExitAutoFrontThread)
	{
		ApplyAutoCorrection(eFront);
		
		Sleep(m_fAutoPeriodMs);
	}

	//Restore camera gain
	pCamera->SetGain(prevGain);

	LOG4CXX_INFO(logger, "AutoFrontLoop is complete");

	return 0;
}

UINT CLightControl::AutoSideLoop()
{
	m_CameraIntegralSat[eLeft] = 0;
	m_CameraIntegralAvg[eLeft] = 0;
	m_CameraIntegralSat[eRight] = 0;
	m_CameraIntegralAvg[eRight] = 0;
	m_CameraIntegralSat[eTop] = 0;
	m_CameraIntegralAvg[eTop] = 0;
	m_CameraIntegralSat[eBottom] = 0;
	m_CameraIntegralAvg[eBottom] = 0;

	//Store and turn off camera auto gain
	vector<bool> prevAutoGain = vector<bool>(5);
	vector<int> prevGain = vector<int>(5);
	for (int i = 0; i < 5; i++)
	{
		if ((CameraType)i != eFront)
		{
			CameraData *pCamera = theApp.GetSystemManager()->GetCameraData((CameraType)i);
			prevAutoGain[i] = pCamera->IsAutoGainEnabled();
			prevGain[i] = pCamera->GetGain();
			pCamera->EnableAutoGain(false);

			_autoStartTimes[i] = clock();
		}
	}

	
	while (!m_bExitAutoSideThread)
	{
		ApplyAutoCorrection(eLeft);
		ApplyAutoCorrection(eRight);
		ApplyAutoCorrection(eTop);
		ApplyAutoCorrection(eBottom);

		Sleep(m_fAutoPeriodMs);
	}

	//Restore camera auto gain
	for (int i = 0; i < 5; i++)
	{
		if ((CameraType)i != eFront)
		{
			CameraData *pCamera = theApp.GetSystemManager()->GetCameraData((CameraType)i);
			pCamera->EnableAutoGain(prevAutoGain[i]);
			pCamera->SetGain(prevGain[i]);
		}
	}

	LOG4CXX_INFO(logger, "AutoSideLoop is complete");

	return 0;
}

void CLightControl::ApplyAutoCorrection(CameraType camera)
{
	AutoLightData data;

	data.Seconds = float(clock() - _autoStartTimes[(int)camera]) / CLOCKS_PER_SEC;

	CameraData *pCamera = theApp.GetSystemManager()->GetCameraData(camera);
	if (pCamera == NULL)
		return;

	if (!pCamera->IsIgnoringFrames())
	{
		double red, green, blue;
		double adjustment = 0;
		double dt = m_fAutoPeriodMs / 1000;

		// For now just use P of PID loop.  Can add I and D if needed
		if (m_bAutoSatOrAvg)
		{
			pCamera->GetLastSaturationValues(red, green, blue);
			double error = m_fAutoSatSetpoint - green; // Use green channel
			m_CameraIntegralSat[camera] += (error * dt);
			m_CameraIntegralSat[camera] = min(m_CameraIntegralSat[camera], 0.1);
			m_CameraIntegralSat[camera] = max(m_CameraIntegralSat[camera], -0.1);

			float deadBand = min(0.10f, m_fAutoSatSetpoint);
			if (abs(error) < deadBand)
			{
				adjustment = 0;

				LOG4CXX_DEBUG(logger, CameraNames[camera] << " is in dead band of " << deadBand);
				//LOG4CXX_DEBUG(logger, printf("%s is in dead band of %.6f", CameraNames[camera], deadBand)); //javqui
			}
			else
			{
				double adjustmentP = m_fAutoSatGainP * error;
				double adjustmentI = m_fAutoSatGainI * m_CameraIntegralSat[camera];
				double adjustmentD = m_fAutoSatGainD * (m_CameraPrevSat[camera] - error) / dt;
				adjustment = adjustmentP + adjustmentI + adjustmentD;

				LOG4CXX_DEBUG(logger, CameraNames[camera] << " AdjP=" << adjustmentP << " AdjI=" << adjustmentI << " AdjD=" << adjustmentD);
				//LOG4CXX_DEBUG(logger, printf("%s AdjP=%.6f AdjI=%.6f AdjD=%.6f",CameraNames[camera], adjustmentP, adjustmentI, adjustmentD)); // javqui
			}

			m_CameraPrevSat[camera] = error;

			LOG4CXX_DEBUG(logger, CameraNames[camera] << " Sat Green=" << green << " SP=" << m_fAutoSatSetpoint << " Adj=" << adjustment);
			//LOG4CXX_DEBUG(logger, printf("%s Sat Green=%.6f SP=%.6f  Adj=%.6f",CameraNames[camera],green, m_fAutoSatSetpoint,adjustment));

			data.SatOrAverage = true;
			data.SetPoint = m_fAutoSatSetpoint;
			data.ValueR = red;
			data.ValueG = green;
			data.ValueB = blue;
			data.Error = error;
			data.DeadBand = deadBand;
			data.P = m_fAutoSatGainP;
			data.I = m_fAutoSatGainI;
			data.D = m_fAutoSatGainD;
			data.Adjustment = adjustment;
		}
		else
		{
			pCamera->GetLastChannelValues(red, green, blue);
			double error = m_fAutoAvgSetpoint - red; // Use red channel since it should be highest
			m_CameraIntegralAvg[camera] += (error * dt);
			m_CameraIntegralSat[camera] = min(m_CameraIntegralSat[camera], 10.0);
			m_CameraIntegralSat[camera] = max(m_CameraIntegralSat[camera], -10.0);

			float deadBand = min(20.0f, m_fAutoAvgSetpoint);
			if (abs(error) < deadBand)
			{
				adjustment = 0;
				//LOG4CXX_DEBUG(logger, CameraNames[camera] << " is in dead band of " << deadBand)
				LOG4CXX_DEBUG(logger, "%s is in dead band of %.6f",CameraNames[camera],deadBand);
			}
			else
			{
				double adjustmentP = m_fAutoAvgGainP * error;
				double adjustmentI = m_fAutoAvgGainI * m_CameraIntegralAvg[camera];
				double adjustmentD = m_fAutoAvgGainD * (error - m_CameraPrevAvg[camera]) / dt;
				adjustment = adjustmentP + adjustmentI + adjustmentD;

				LOG4CXX_DEBUG(logger, CameraNames[camera] << " AdjP=" << adjustmentP << " AdjI=" << adjustmentI << " AdjD=" << adjustmentD);
				//LOG4CXX_DEBUG(logger, printf("%s AdjP=%.6f AdjI=%.6f AdjD=%.6f",CameraNames[camera],adjustmentP,adjustmentI,adjustmentD));

			}

			m_CameraPrevAvg[camera] = error;

			LOG4CXX_DEBUG(logger, CameraNames[camera] << " Avg Red=" << red << " SP=" << m_fAutoAvgSetpoint << " Adj=" << adjustment);
			//LOG4CXX_DEBUG(logger, printf("%s Avg Red=%.6f SP=%.6f Adj=%.6f", CameraNames[camera], red, m_fAutoSatSetpoint, adjustment));

			data.SatOrAverage = false;
			data.SetPoint = m_fAutoAvgSetpoint;
			data.ValueR = red;
			data.ValueG = green;
			data.ValueB = blue;
			data.Error = error;
			data.DeadBand = deadBand;
			data.P = m_fAutoAvgGainP;
			data.I = m_fAutoAvgGainI;
			data.D = m_fAutoAvgGainD;
			data.Adjustment = adjustment;
		}

#if 0 // Increment gain up and down by one index
		float valLights = GetCameraValue(camera, LEFT);
		float newLightsVal = valLights + adjustment;
		newLightsVal = max(newLightsVal, 0.0f);
		newLightsVal = min(newLightsVal, 1.0f);

		LOG4CXX_DEBUG(logger, CameraNames[camera] << " OldLightsVal=" << valLights << " NewLightsVal=" << newLightsVal);

		data.PrevLight = valLights;
		data.NewLight = newLightsVal;

		if (valLights != newLightsVal)
		{
			if (m_AutoMode == LIGHTS)
			{
				SetCameraValue(camera, LEFT, newLightsVal, true);
				data.GainInc = 0;
			}
			else if (m_AutoMode == BOTH)
			{
				bool changed = false;
				if (newLightsVal <= MIN_LIGHT_GAIN_CHANGE)
				{
					// Raise the gain
					changed = theApp.GetSystemManager()->GetCameraData(camera)->ChangeGainIndex(-1);
					data.GainInc = -1;
				}
				if (newLightsVal >= MAX_LIGHT_GAIN_CHANGE)
				{
					// Lower the gain the gain
					changed = theApp.GetSystemManager()->GetCameraData(camera)->ChangeGainIndex(+1);
					data.GainInc = +1;
				}

				if (!changed)
				{
					SetCameraValue(camera, LEFT, newLightsVal, true);
					data.GainInc = 0;
				}
			}
		}
#else
		if (m_AutoMode == LIGHTS)
		{
			float valLights = GetCameraValue(camera, LEFT);
			float newLightsVal = valLights + adjustment;
			newLightsVal = max(newLightsVal, 0.0f);
			newLightsVal = min(newLightsVal, 1.0f);

			LOG4CXX_DEBUG(logger, CameraNames[camera] << " OldLightsVal=" << valLights << " NewLightsVal=" << newLightsVal);
			//LOG4CXX_DEBUG(logger, printf("%s OldLightsVal=%.6f NewLightsVal=%.6f",CameraNames[camera],valLights,newLightsVal));

			data.PrevLight = valLights;
			data.NewLight = newLightsVal;

			if (valLights != newLightsVal)
			{
				SetCameraValue(camera, LEFT, newLightsVal, true);
			}
		}
		else
		{
			int valGain = pCamera->GetGain();
			int newValGain = valGain + (int)round(50.0*adjustment);
			int maxVal = pCamera->GetGainMaxIndexValue();
			
			if (camera != eFront)
			{
				newValGain = valGain + (int)round(50.0*adjustment);
				if ((_gainLookup.size() > 0) && (camera != eFront))
				{
					maxVal = _gainLookup[_gainLookup.size() - 1];
					//Find current index
					int index = 1;
					while (index < _gainLookup.size())
					{
						if (_gainLookup[index] > newValGain)
						{
							break;
						}
						index++;
					}
					index += (int)round(10.0*adjustment);
					index = max(index, 0);
					index = min(index, (int)_gainLookup.size() - 1);
					newValGain = _gainLookup[index];
				}
			}
			newValGain = max(newValGain, 0);
			newValGain = min(newValGain, maxVal);

			LOG4CXX_DEBUG(logger, CameraNames[camera] << " OldGainVal=" << valGain << " NewGainVal=" << newValGain);
			//LOG4CXX_DEBUG(logger, printf("%s OldGainVal%.6f= NewGainVal=%.6f", CameraNames[camera],valGain,newValGain));


			data.PrevGain = valGain;
			data.NewGainRaw = newValGain;

			// Use the closest gain index value
			/*
			int gainIndex = pCamera->FindGainIndex(newValGain, (adjustment > 0));
			newValGain = pCamera->GetGainIndexValue(gainIndex);
			data.NewGainIndex = newValGain;
			*/
			float valLights = GetCameraValue(camera, LEFT);
			float newLightsVal = valLights;
			if (m_AutoMode == BOTH)
			{
				newLightsVal = valLights + adjustment;
				newLightsVal = max(newLightsVal, 0.0f);
				newLightsVal = min(newLightsVal, 1.0f);

				//LOG4CXX_DEBUG(logger, CameraNames[camera] << " OldLightsVal=" << valLights << " NewLightsVal=" << newLightsVal);
				LOG4CXX_DEBUG(logger, "%s  OldLightsVal=%.6f  NewLightsVal=%.6f",CameraNames[camera],valLights,newLightsVal);
			}
			
			data.PrevLight = valLights;
			data.NewLight = newLightsVal;

			if (adjustment > 0)
			{
				// Brighter - increase lights and then gain
				if (valLights != newLightsVal)
				{
					SetCameraValue(camera, LEFT, newLightsVal, true);
				}
				else if (valGain != newValGain)
				{
					theApp.GetSystemManager()->GetCameraData(camera)->SetGain(newValGain);
				}
			}
			else if (adjustment < 0)
			{
				// Dimmer - decrease gain and then lights
				if (valGain != newValGain)
				{
					theApp.GetSystemManager()->GetCameraData(camera)->SetGain(newValGain);
				}
				else if (valLights != newLightsVal)
				{
					SetCameraValue(camera, LEFT, newLightsVal, true);
				} 
			}
		}
#endif
	}

	if (_autoLightData[(int)camera].size() > 500)
	{
		_autoLightData[(int)camera].pop();
	}
	_autoLightData[(int)camera].push(data);
}

void CLightControl::EnableSelectBandImaging(bool enable)
{
	m_bSelectBandImaging = enable;
	m_bCustomColorEnabled = false;


	// javqui
	std::map<int, float> values;
	if (m_bSelectBandImaging)
	{
		
		values[6] = 0;  // white top
		values[24] = 0; // white top
		values[12] = 0; // white side 1
		values[18] = 0; // white side 1
		values[0] = 0;  // white side 2
		values[7] = 0;  // white side 2

		values[8] = 0x0FF;  // blue top
		values[26] = 0x0FF; // blue top
		values[20] = 0x0ff; // blue side 1
		values[13] = 0x0ff; // blue side 1
		values[2] = 0x0FF;  // blue side 2
		values[14] = 0x0fF;  // blue side 2


		values[25] = 0x0FF;  // green top
		values[19] = 0xFF; // green side 1
		values[1] = 0xFF; // green side 2
		
		
		// BLUE + GREEN   side normal
		/*values[6] = 0;  // white top
		values[24] = 0; // white top
		values[12] = 0; // white side 1
		values[18] = 0; // white side 1
		values[0] = 0;  // white side 2
		values[7] = 0;  // white side 2

		values[8] = 0xBB;  // blue top
		values[26] = 0xBB; // blue top
		values[20] = 0xAA; // blue side 1
		values[13] = 0xAA; // blue side 1
		values[2] = 0xAA;  // blue side 2
		values[14] = 0xAA;  // blue side 2


		values[25] = 0xFF;  // green top
		values[19] = 0xCC; // green side 1
		values[1] = 0xCC; // green side 2
		*/
		
	
		


	}
	else {
		
		// WHITE ONLY   side normal
		values[6] = 0x80;  // white top
		values[24] = 0x80; // white top
		values[12] = 0; // white side 1
		values[18] = 0; // white side 1
		values[0] = 0;  // white side 2
		values[7] = 0;  // white side 2

		values[8] = 0;  // blue top
		values[26] = 0; // blue top
		values[20] = 0; // blue side 1
		values[13] = 0; // blue side 1
		values[2] = 0;  // blue side 2
		values[14] = 0;  // blue side 2


		values[25] = 0;  // green top
		values[19] = 0; // green side 1
		values[1] = 0; // green side 2
		/*
		values[6] = 0x80;  // white top
		values[24] = 0x80; // white top
		values[12] = 0xA0; // white side 1
		values[18] = 0xA0; // white side 1
		values[0] = 0x80;  // white side 2
		values[7] = 0x80;  // white side 2

		values[8] = 0;  // blue top
		values[26] = 0; // blue top
		values[20] = 0; // blue side 1
		values[13] = 0; // blue side 1
		values[2] = 0;  // blue side 2
		values[14] = 0;  // blue side 2


		values[25] = 0;  // green top
		values[19] = 0; // green side 1
		values[1] = 0; // green side 2
		*/


	}


	UpdateChannelValues(values, true);
/*

	for (int camera = 0; camera < 5; camera++)
	{
		for (int side = 0; side < 2; side++)
		{
			// Get the current intensity
			float value = GetCameraValue((CameraType)camera, (LightSide)side);

			// Update with the new color
			SetCameraValue((CameraType)camera, (LightSide)side, value, true);
		}
	}
*/
	std::map<SelectBandImagingCallbackType, void*>::iterator it;
	for (it = m_SelectBandImagingCallbacks.begin(); it != m_SelectBandImagingCallbacks.end(); ++it)
	{
		it->first(it->second, enable);
	}


}

void CLightControl::SetCustomColor(COLORREF color)
{
	if (color == 0)
	{
		m_bCustomColorEnabled = false;
	}
	else
	{
		m_CustomColor = color;
		m_bCustomColorEnabled = true;
	}

	for (int camera = 0; camera < 5; camera++)
	{
		for (int side = 0; side < 2; side++)
		{
			// Get the current intensity
			float value = GetCameraValue((CameraType)camera, (LightSide)side);

			// Update with the new color
			SetCameraValue((CameraType)camera, (LightSide)side, value, true);
		}
	}
}

void CLightControl::ToggleDemoMode()
{
	if (m_hDemoThread)
	{
		StopDemoThread();
	}
	else
	{ 
		// Create the auto thread
		DWORD dwThreadID;
		m_hDemoThread = CreateThread(
			NULL,         // default security attributes
			0,            // default stack size
			(LPTHREAD_START_ROUTINE)&CLightControl::DemoThreadProc,
			(LPVOID)this,         // no thread function arguments
			0,            // default creation flags
			&dwThreadID); // receive thread identifier
	}
}

void CLightControl::StopDemoThread()
{
	if (m_hDemoThread != NULL)
	{
		m_bExitDemoThread = true;
		WaitForSingleObject(m_hDemoThread, INFINITE);
		m_bExitDemoThread = false;
		m_hDemoThread = NULL;
	}
}

UINT CLightControl::DemoLoop()
{
	int camera, side, color;

	// Turn off all the cameras and lights
	for (camera = 0; camera < 5; camera++)
	{
		for (side = 0; side < 2; side++)
		{
			SetCameraValue((CameraType)camera, (LightSide)side, 0, true);
		}
	}

	// Turn on the first set of lights
	camera = 0;
	side = 0;
	color = 0;
	DemoLEDValue(camera, side, color, 1.0, true);
	Sleep(DEMO_CYCLE_MS);

	while (!m_bExitDemoThread)
	{
		DemoLEDValue(camera, side, color, 0.0, true);
		color++;
		if (color == 3)
		{
			color = 0;
			side++;
			if (side == 1) // Side 0 controls the brightness of both LEDs
			{
				side = 0;
				camera++;
				if (camera == 5)
					camera = 0;
			}
		}
		DemoLEDValue(camera, side, color, 1.0, true);

		Sleep(DEMO_CYCLE_MS);
	}

	return 0;
}

void CLightControl::DemoLEDValue(int camera, int side, int color, float value, bool bUpdateMC)
{
	int channel = LightChannelLookup((CameraType) camera, (LightSide)side, (Color)color);
	UpdateChannelValue(channel, value, bUpdateMC);

	if (m_UpdateLightCallback)
	{
		m_UpdateLightCallback(m_pUpdateCallbackObject, (CameraType)camera, (LightSide)side, (int)(float)(100.0*value + 0.5));
	}
}

void CLightControl::SaveAutoLightData(string folder)
{
	for (int i = 0; i < 5; i++)
	{
		std::ofstream file(folder + "\\" + CameraNames[i] + "_AutoLights.csv");
		file << AutoLightData::GetHeader() << std::endl;
		while (_autoLightData[i].size() > 0)
		{
			AutoLightData data = _autoLightData[i].front();
			_autoLightData[i].pop();
			file << data.GetData() << std::endl;
		}
		file.close();
	}
}

bool CLightControl::IsSingleColor(COLORREF color)
{
	int numComponents = 0;
	for (int i = 0; i < 3; i++)
	{
		int val = ((color >> i) & 0xff);
		if (val != 0)
		{
			numComponents++;
		}
	}

	return (numComponents == 1);

}


string AutoLightData::GetHeader()
{
	return "Seconds,SatOrAverage,SetPoint,ValueR,ValueG,ValueB,P,I,D,Error,Adjustment,DeadBand,PrevLight,NewLight,GainInc,PrevGain,NewGainRaw,NewGainIndex";
}

string AutoLightData::GetData()
{
	std::stringstream   ss;
	ss << Seconds << "," 
		<< (SatOrAverage ? "1" : "0") << ","
		<< SetPoint << ","
		<< ValueR << ","
		<< ValueG << ","
		<< ValueB << ","
		<< P << ","
		<< I << ","
		<< D << ","
		<< Error << ","
		<< Adjustment << ","
		<< DeadBand << ","
		<< PrevLight << ","
		<< NewLight << ","
		<< GainInc << ","
		<< PrevGain << ","
		<< NewGainRaw << ","
		<< NewGainIndex;

	return ss.str();
}
