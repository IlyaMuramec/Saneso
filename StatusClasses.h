#pragma once

#include "json\json.h"
#include "opencv2/core/core.hpp"

const std::string CameraNames[] = { "Front", "Left", "Right", "Top", "Bottom" };

enum CameraType
{
	eFront,
	eLeft,
	eRight,
	eTop,
	eBottom
};

enum CompStatus
{
	eUnknown,
	eOK,
	eMarginal,
	eError,
	eSimulated,
	eFatal,
	eCalibrating
};

enum Color
{
	RED,
	GREEN,
	BLUE
};

enum Measurement
{
	eNoMeasurement,
	eDeviation,
	eLightResponse
};
const std::string MeasurementNames[] = { "NoMeasurement", "Deviation", "LightResponse" };

class SystemStatus
{
public:
	SystemStatus();
	CompStatus GetOverallSystemStatus();
	bool AreAllCamerasOK();
	void Save(Json::Value &root);

	static std::string GetCompStatusString(CompStatus status);

	double m_dFPSMosaic;
	double m_dFPSFrontCamera;
	double m_dFPSTopCamera;
	double m_dFPSBottomCamera;
	double m_dFPSLeftCamera;
	double m_dFPSRightCamera;
	double m_dControllerHeartBeat;
	std::string m_ControllerVersion;
	std::string m_ControllerStatusString;

	CompStatus m_MosaicStatus;
	std::vector<CompStatus> m_CamerasStatus;
	CompStatus m_USBStatus;
	CompStatus m_ControllerStatus;
	CompStatus m_SelfCalibrationStatus;
};

class SelfCalibrationSettings
{
public:
	SelfCalibrationSettings();

	void Save(Json::Value &root);

	double m_dCalCupMaxDeviation;
	float m_fCalCupFrontLightLevel;
	float m_fCalCupSideLightLevel;

	int m_nLightCheckShadowCastDelay;
	int m_nLightCheckTime;
	float m_fLightCheckMinValue;

	int m_nFPNTimeoutSecs;
	int m_nFPNMaxValueLimit;
	int m_nWBSettleDelay;

	float m_fWBFrontCompRed;
	float m_fWBFrontCompGreen;
	float m_fWBFrontCompBlue;
	float m_fWBSideCompRed;
	float m_fWBSideCompGreen;
	float m_fWBSideCompBlue;
	float m_fWBTolerance;

	int m_nCameraExposure;
	int m_nCameraGain;
};

class SelfCalibrationStatus
{
public:
	SelfCalibrationStatus();

	static std::string GetTimestamp(time_t time);
	static time_t FromTimestamp(std::string str);

	static std::string GetCSVHeader();
	std::string GetCSVLine();
	void FromCSVLine(std::string line);

	void SaveJson(std::string fileName);

	time_t m_Time;

	SystemStatus m_SystemStatus;
	SelfCalibrationSettings m_Settings;
	std::vector<std::vector<double>> m_LightResponses;
	std::vector<double> m_CalCupDeviations;
	std::vector<double> m_FPNMaxValues;
	std::vector<std::vector<double>> m_WhiteBalanceGains;
	std::vector<cv::Mat> m_ImagesCalCup;

	CompStatus m_CamerasCalStatus;
	CompStatus m_LightsCalStatus;
	CompStatus m_CalCupCalStatus;
	CompStatus m_FPNCalStatus;
	CompStatus m_WBCalStatus;
	CompStatus m_OverallCalStatus;
};

struct CalStatuses
{
	CompStatus CamerasCalStatus;
	CompStatus LightsCalStatus;
	CompStatus CalCupCalStatus;
	CompStatus FPNCalStatus;
	CompStatus WBCalStatus;
	CompStatus OverallCalStatus;
};
