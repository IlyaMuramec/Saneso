#include "stdafx.h"
#include "ProcessorBoxSettings.h"
#include "json\json.h"

using namespace std;

log4cxx::LoggerPtr  CProcessorBoxSettings::logger(log4cxx::Logger::getLogger("ProcessorBoxSettings"));

CProcessorBoxSettings::CProcessorBoxSettings()
{
	m_bFrontSplitImageDetectionEnabled = true;
	m_bScopeCalibrationRequired = true;
}

CProcessorBoxSettings::CProcessorBoxSettings(std::string jsonEEProm)
{
	m_bFrontSplitImageDetectionEnabled = true;
	m_bScopeCalibrationRequired = true;

	UpdateFromEEPromString(jsonEEProm);
}

CProcessorBoxSettings::~CProcessorBoxSettings()
{
}


std::string CProcessorBoxSettings::GetEEPromString()
{
	Json::Value root;
	
	root["Front Split Image Detection Enabled"] = m_bFrontSplitImageDetectionEnabled;
	root["Scope Calibration Required"] = m_bScopeCalibrationRequired;

	Json::FastWriter writer;
	return writer.write(root);
}

void CProcessorBoxSettings::UpdateFromEEPromString(std::string jsonEEProm)
{
	Json::Reader reader;
	Json::Value root;
	bool parsingSuccessful = reader.parse(jsonEEProm, root);
	if (parsingSuccessful)
	{
		m_bFrontSplitImageDetectionEnabled = root.get("Front Split Image Detection Enabled", true).asBool();
		m_bScopeCalibrationRequired = root.get("Scope Calibration Required", true).asBool();
	}
	else
	{
		LOG4CXX_INFO(logger, "Unable to create Processor Box settings from EEPROM data");
	}

	if (!m_bFrontSplitImageDetectionEnabled)
	{
		LOG4CXX_ERROR(logger, "Front Split Image Detection is not enabled!");
	}

	if (!m_bScopeCalibrationRequired)
	{
		LOG4CXX_ERROR(logger, "Scope calibration is not required!");
	}
}


