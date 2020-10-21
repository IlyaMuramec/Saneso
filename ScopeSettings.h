#pragma once

#include <string>
#include <vector>
#include <log4cxx\logger.h>

class CCameraSettings;
class CLightCalSettings;

class CScopeSettings
{
public:
	CScopeSettings(std::string serialNumber, std::vector<CCameraSettings*> cameraSettings, CLightCalSettings* lightCalSettings);
	CScopeSettings(std::string jsonEEProm);
	~CScopeSettings();

	std::string GetEEPromString();
	void UpdateFromEEPromString(std::string jsonEEProm);

	int GetNumCameras() { return _cameraSettings.size();  }

private:
	static log4cxx::LoggerPtr logger;

	bool _ownsMemory;
	std::vector<CCameraSettings*> _cameraSettings;
	CLightCalSettings* _lightCalSettings;
};

