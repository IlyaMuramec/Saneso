#pragma once

#include <string>
#include <vector>
#include <log4cxx\logger.h>

class CCameraSettings;
class CLightCalSettings;

class CProcessorBoxSettings
{
public:
	CProcessorBoxSettings();
	CProcessorBoxSettings(std::string jsonEEProm);
	~CProcessorBoxSettings();

	std::string GetEEPromString();
	void UpdateFromEEPromString(std::string jsonEEProm);

	bool IsFrontSplitImageDetectionEnabled() { return m_bFrontSplitImageDetectionEnabled; }
	bool IsScopeCalibrationRequired() { return m_bScopeCalibrationRequired; }

private:
	static log4cxx::LoggerPtr logger;

	bool m_bFrontSplitImageDetectionEnabled;
	bool m_bScopeCalibrationRequired;
};

