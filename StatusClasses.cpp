#include "stdafx.h"
#include "StatusClasses.h"
#include "NGEuserApp.h"
#include <fstream>
#include <iostream>
#include <ctime>

using namespace std;

string CompStatusToString(CompStatus state)
{
	string str = "Unknown";

	switch (state)
	{
	case eOK: str = "OK"; break;
	case eMarginal: str = "Marginal"; break;
	case eError: str = "Error"; break;
	case eFatal: str = "Fatal"; break;
	}

	return str;
}

CompStatus StringToCompStatus(string str)
{
	CompStatus state = eUnknown;

	if (str == "OK")
	{
		state = eOK;
	}
	else if (str == "Marginal")
	{
		state = eMarginal;
	}
	else if (str == "Error")
	{
		state = eError;
	}
	else if (str == "Fatal")
	{
		state = eFatal;
	}

	return state;
}

SystemStatus::SystemStatus()
{
	m_CamerasStatus = std::vector<CompStatus>(5);
}

std::string SystemStatus::GetCompStatusString(CompStatus status)
{
	switch (status)
	{
	case eOK: 
		return "OK"; 
		break;
	case eMarginal: 
		return "Marginal"; 
		break;
	case eError: 
		return "Error"; 
		break;
	case eFatal:
		return "Fatal"; 
		break;
	case eCalibrating:
		return "Calibrating";
		break;
	default: 
		return "Unknown";
	}
}


CompStatus SystemStatus::GetOverallSystemStatus()
{
	CompStatus status = eError;

	if (AreAllCamerasOK()
		&& (m_ControllerStatus == eOK)
		&& (m_MosaicStatus == eOK))
	{
		status = eOK;
	}

	status = max(status, m_USBStatus);

	status = max(status, m_SelfCalibrationStatus);
		
	return status;
}

bool SystemStatus::AreAllCamerasOK()
{
	bool bAllOK = true;

	for (int i = 0; i < m_CamerasStatus.size(); i++)
	{
		if (m_CamerasStatus[i] == eSimulated)
			continue;

		if (m_CamerasStatus[i] != eOK)
		{
			bAllOK = false;
			break;
		}
	}

	return bAllOK;
}

void SystemStatus::Save(Json::Value &root)
{
	root["FPSMosaic"] = (float)m_dFPSMosaic;
	root["FPSFrontCamera"] = (float)m_dFPSFrontCamera;
	root["FPSTopCamera"] = (float)m_dFPSTopCamera;
	root["FPSSBottomCamera"] = (float)m_dFPSBottomCamera;
	root["FPSLeftCamera"] = (float)m_dFPSLeftCamera;
	root["FPSRightCamera"] = (float)m_dFPSRightCamera;
	root["ControllerHeartBeat"] = (float)m_dControllerHeartBeat;
	root["ControllerVersion"] = m_ControllerVersion;
	root["ControllerStatus"] = m_ControllerStatusString;


	root["StatusMosaic"] = CompStatusToString(m_MosaicStatus);
	root["StatusFrontCamera"] = CompStatusToString(m_CamerasStatus[eFront]);
	root["StatusTopCamera"] = CompStatusToString(m_CamerasStatus[eTop]);
	root["StatusBottomCamera"] = CompStatusToString(m_CamerasStatus[eBottom]);
	root["StatusLeftCamera"] = CompStatusToString(m_CamerasStatus[eLeft]);
	root["StatusRightCamera"] = CompStatusToString(m_CamerasStatus[eRight]);
	root["StatusController"] = CompStatusToString(m_ControllerStatus);
	root["StatusUSB"] = CompStatusToString(m_USBStatus);
}

SelfCalibrationSettings::SelfCalibrationSettings()
{
	m_dCalCupMaxDeviation = 30;
	m_fCalCupFrontLightLevel = 0.2f;
	m_fCalCupSideLightLevel = 0.2f;
	m_nLightCheckShadowCastDelay = 1500;
	m_nLightCheckTime = 500;
	m_fLightCheckMinValue = 10.0f;
	m_nFPNTimeoutSecs = 15;
	m_nFPNMaxValueLimit = 2000;
	m_nWBSettleDelay = 500;
	m_fWBFrontCompRed = 1.0f;
	m_fWBFrontCompGreen = 1.0f;
	m_fWBFrontCompBlue = 1.0f;
	m_fWBSideCompRed = 1.25f;
	m_fWBSideCompGreen = 1.0f;
	m_fWBSideCompBlue = 1.0f;
	m_fWBTolerance = 0.25f;
	m_nCameraExposure = 30;
	m_nCameraGain = 0;
}

void SelfCalibrationSettings::Save(Json::Value &root)
{
	root["CalCupMaxDeviation"] = (float)m_dCalCupMaxDeviation;
	root["CalCupFrontLightLevel"] = m_fCalCupFrontLightLevel;
	root["CalCupSideLightLevel"] = m_fCalCupSideLightLevel;
	root["LightCheckShadowCastDelay"] = m_nLightCheckShadowCastDelay;
	root["LightCheckTime"] = m_nLightCheckTime;
	root["LightCheckMinValue"] = m_fLightCheckMinValue;
	root["FPNTimeoutSecs"] = m_nFPNTimeoutSecs;
	root["FPNMaxValueLimit"] = m_nFPNMaxValueLimit;
	root["WBSettleDelay"] = m_nWBSettleDelay;
	root["WBFrontCompRed"] = m_fWBFrontCompRed;
	root["WBFrontCompGreen"] = m_fWBFrontCompGreen;
	root["WBFrontCompBlue"] = m_fWBFrontCompBlue;
	root["WBSideCompRed"] = m_fWBSideCompRed;
	root["WBSideCompGreen"] = m_fWBSideCompGreen;
	root["WBSideCompBlue"] = m_fWBSideCompBlue;
	root["WBTolerance"] = m_fWBTolerance;
}


SelfCalibrationStatus::SelfCalibrationStatus()
{
	std::time(&m_Time);

	m_CalCupDeviations.resize(5);
	m_FPNMaxValues.resize(5, DBL_MAX);
	m_LightResponses.resize(5);
	m_WhiteBalanceGains.resize(5);
	for (int i = 0; i < m_LightResponses.size(); i++)
	{
		m_LightResponses[i].resize(3);
		m_WhiteBalanceGains[i].resize(3);
	}
		
	m_CamerasCalStatus = eUnknown;
	m_LightsCalStatus = eUnknown;
	m_CalCupCalStatus = eUnknown;
	m_FPNCalStatus = eUnknown;
	m_WBCalStatus = eUnknown;
	m_OverallCalStatus = eUnknown;
}

std::string SelfCalibrationStatus::GetTimestamp(time_t time)
{
	char buf[80];
	struct tm * timeinfo = localtime(&time);

	strftime(buf, 80, "%Y%m%d_%H%M%S", timeinfo);

	return string(buf);
}

time_t SelfCalibrationStatus::FromTimestamp(std::string str)
{
	tm timeinfo;

	sscanf(str.c_str(), "%04d%02d%02d_%02d%02d%02d",
		&timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday,
		&timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);

	timeinfo.tm_year -= 1900;
	timeinfo.tm_mon -= 1;

	return mktime(&timeinfo);
}

std::string SelfCalibrationStatus::GetCSVHeader()
{
	return "Time,Overall,Cameras,CalCup,Lights,FPN,WB";
}

std::string SelfCalibrationStatus::GetCSVLine()
{
	char buf[80];
	struct tm * timeinfo = localtime(&m_Time);

	strftime(buf, 80, "%Y%m%d_%H%M%S", timeinfo);

	string str = buf;
	str += "," + CompStatusToString(m_OverallCalStatus);
	str += "," + CompStatusToString(m_CamerasCalStatus);
	str += "," + CompStatusToString(m_CalCupCalStatus);
	str += "," + CompStatusToString(m_LightsCalStatus);
	str += "," + CompStatusToString(m_FPNCalStatus);
	str += "," + CompStatusToString(m_WBCalStatus);

	return str;
}

void SelfCalibrationStatus::FromCSVLine(std::string line)
{
	std::string delimiter = ",";

	try
	{
		int index = 0;
		size_t pos = 0;
		std::string token;
		while ((pos = line.find(delimiter)) != std::string::npos)
		{
			token = line.substr(0, pos);

			switch (index)
			{
			case 0: m_Time = FromTimestamp(token); break;
			case 1: m_OverallCalStatus = StringToCompStatus(token); break;
			case 2: m_CamerasCalStatus = StringToCompStatus(token); break;
			case 3: m_CalCupCalStatus = StringToCompStatus(token); break;
			case 4: m_LightsCalStatus = StringToCompStatus(token); break;
			case 5: m_FPNCalStatus = StringToCompStatus(token); break;
			case 6: m_WBCalStatus = StringToCompStatus(token); break;
			}

			line.erase(0, pos + delimiter.length());
			index++;
		}
	}
	catch (...)
	{
		string msg = "Error parsing last calibration info\n";
		msg += line;
		theApp.ShowMessageBox(msg.c_str(), "Error", MB_OK | MB_TOPMOST);

		// Reset
		*this = SelfCalibrationStatus();
	}
}

void SelfCalibrationStatus::SaveJson(string fileName)
{
	Json::Value root;

	root["Time"] = GetTimestamp(m_Time);

	root["StatusOverall"] = CompStatusToString(m_OverallCalStatus);
	root["StatusCameras"] = CompStatusToString(m_CamerasCalStatus);
	root["StatusCalCup"] = CompStatusToString(m_CalCupCalStatus);
	root["StatusLights"] = CompStatusToString(m_LightsCalStatus);
	root["StatusFPN"] = CompStatusToString(m_FPNCalStatus);
	root["StatusWB"] = CompStatusToString(m_WBCalStatus);

	root["Lights.Front.Red"] = (float)m_LightResponses[eFront][RED];
	root["Lights.Front.Green"] = (float)m_LightResponses[eFront][GREEN];
	root["Lights.Front.Blue"] = (float)m_LightResponses[eFront][BLUE];

	root["Lights.Top.Red"] = (float)m_LightResponses[eTop][RED];
	root["Lights.Top.Green"] = (float)m_LightResponses[eTop][GREEN];
	root["Lights.Top.Blue"] = (float)m_LightResponses[eTop][BLUE];

	root["Lights.Bottom.Red"] = (float)m_LightResponses[eBottom][RED];
	root["Lights.Bottom.Green"] = (float)m_LightResponses[eBottom][GREEN];
	root["Lights.Bottom.Blue"] = (float)m_LightResponses[eBottom][BLUE];
	
	root["Lights.Left.Red"] = (float)m_LightResponses[eLeft][RED];
	root["Lights.Left.Green"] = (float)m_LightResponses[eLeft][GREEN];
	root["Lights.Left.Blue"] = (float)m_LightResponses[eLeft][BLUE];

	root["Lights.Right.Red"] = (float)m_LightResponses[eRight][RED];
	root["Lights.Right.Green"] = (float)m_LightResponses[eRight][GREEN];
	root["Lights.Right.Blue"] = (float)m_LightResponses[eRight][BLUE];

	root["WB.Front.Red"] = (float)m_WhiteBalanceGains[eFront][RED];
	root["WB.Front.Green"] = (float)m_WhiteBalanceGains[eFront][GREEN];
	root["WB.Front.Blue"] = (float)m_WhiteBalanceGains[eFront][BLUE];

	root["WB.Top.Red"] = (float)m_WhiteBalanceGains[eTop][RED];
	root["WB.Top.Green"] = (float)m_WhiteBalanceGains[eTop][GREEN];
	root["WB.Top.Blue"] = (float)m_WhiteBalanceGains[eTop][BLUE];

	root["WB.Bottom.Red"] = (float)m_WhiteBalanceGains[eBottom][RED];
	root["WB.Bottom.Green"] = (float)m_WhiteBalanceGains[eBottom][GREEN];
	root["WB.Bottom.Blue"] = (float)m_WhiteBalanceGains[eBottom][BLUE];

	root["WB.Left.Red"] = (float)m_WhiteBalanceGains[eLeft][RED];
	root["WB.Left.Green"] = (float)m_WhiteBalanceGains[eLeft][GREEN];
	root["WB.Left.Blue"] = (float)m_WhiteBalanceGains[eLeft][BLUE];

	root["WB.Right.Red"] = (float)m_WhiteBalanceGains[eRight][RED];
	root["WB.Right.Green"] = (float)m_WhiteBalanceGains[eRight][GREEN];
	root["WB.Right.Blue"] = (float)m_WhiteBalanceGains[eRight][BLUE];
	
	root["CalCup.Front"] = (float)m_CalCupDeviations[eFront];
	root["CalCup.Top"] = (float)m_CalCupDeviations[eTop];
	root["CalCup.Bottom"] = (float)m_CalCupDeviations[eBottom];
	root["CalCup.Left"] = (float)m_CalCupDeviations[eLeft];
	root["CalCup.Right"] = (float)m_CalCupDeviations[eRight];

	root["FPNMax.Front"] = (float)m_FPNMaxValues[eFront];
	root["FPNMax.Top"] = (float)m_FPNMaxValues[eTop];
	root["FPNMax.Bottom"] = (float)m_FPNMaxValues[eBottom];
	root["FPNMax.Left"] = (float)m_FPNMaxValues[eLeft];
	root["FPNMax.Right"] = (float)m_FPNMaxValues[eRight];

	/*m_ImageUnifmormities.resize(5);
	m_FPNMaxValues.resize(5);
	m_WhiteBalanceMaxValues.resize(5);*/

	Json::Value systemStatus;
	m_SystemStatus.Save(systemStatus);
	root["SystemStatus"] = systemStatus;

	Json::Value settings;
	m_Settings.Save(settings);
	root["Settings"] = settings;

	ofstream file(fileName);
	Json::StreamWriterBuilder styledWriterBuilder;
	styledWriterBuilder.settings_["precision"] = 5;
	file << Json::writeString(styledWriterBuilder, root);
	file.close();
}

